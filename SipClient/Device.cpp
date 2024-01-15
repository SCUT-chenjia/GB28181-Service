#include "pch.h"
#include "Device.h"
#include "HttpClient.h"
#include "HttpServer.h"
#include "NetUtils.h"
#include "FileTime.h"
static int start_port = 20000;
static int SN_MAX = 99999999;
static int sn = 0;

static int get_port() {
	start_port++;
	if (start_port > 65500)
	{
		return 20000;
	}
	return start_port;
}

static int get_sn() {
	if (sn >= SN_MAX) {
		sn = 0;
	}
	sn++;
	return sn;
}

#define CALLBACK_TEMPLATE(F) (std::bind(&SipDevice::F, this, std::placeholders::_1))


SipDevice::SipDevice(const std::shared_ptr<DeviceInfo> info, std::shared_ptr<SipServerInfo> sip_server_info) {
	this->ID = info->ID;
	this->IP = info->IP;
	this->Port = info->Port;
	this->Protocol = info->Protocol;
	this->Name = info->Name;
	this->Manufacturer = info->Manufacturer;
	this->HeartbeatInterval = info->HeartbeatInterval;
	this->Channels = info->Channels;

	this->_sip_server_info = sip_server_info;
}

bool SipDevice::init() {
	_sip_context = eXosip_malloc();
	if (OSIP_SUCCESS != eXosip_init(_sip_context)) {
		LOG(ERROR) << "eXosip_init failed";
		return false;
	}
	eXosip_set_option(_sip_context, EXOSIP_OPT_SET_HEADER_USER_AGENT, this->Manufacturer.c_str());

	_from_uri = fmt::format("sip:{}@{}:{}", this->ID, this->IP, this->Port);
	_contact_url = fmt::format("sip:{}@{}:{}", this->ID, this->IP, this->Port);
	_proxy_uri = fmt::format("sip:{}@{}:{}", _sip_server_info->ID, _sip_server_info->IP, _sip_server_info->Port);

	HttpServer::GetInstance()->AddStreamChangedCallback([this](const std::string& app, const std::string& stream, bool regist) {
		//ֻ��Ҫ�ж�¼��طŵ���ע���¼�����
		if (!regist && app == "record")
		{
			std::scoped_lock<std::mutex> g(_session_mutex);
			for (auto&& [id, session] : _session_map)
			{
				if (session->SSRC == stream)
				{
					send_notify(session);

					//��map��ɾ����session����ע�����Զ��ͷ�
					_session_map.erase(id);
					break;
				}
			}
		}
		});

	return true;
}

/// @brief ����sip�ͻ��˹���
/// �󶨱��ض˿ڣ������߳̽������ݣ�����ע����Ϣ
/// @return 
bool SipDevice::start_sip_client() {
	if (NetHelper::IsPortAvailable(this->Port))
		_local_port = this->Port;
	else {
		_local_port = NetHelper::FindAvailablePort();
		LOG(INFO) << fmt::format("�˿�:{}��ռ�ã���ʹ�ö˿ڣ�{}", this->Port, _local_port);
	}

	if (OSIP_SUCCESS != eXosip_listen_addr(_sip_context, this->Protocol, NULL, _local_port, AF_INET, 0)) {
		LOG(ERROR) << "eXosip_listen_addr";
		eXosip_quit(_sip_context);
		return false;
	}

	_is_running = true;
	_sip_thread = std::make_shared<std::thread>(&SipDevice::on_response, this);

	eXosip_clear_authentication_info(_sip_context);

	osip_message_t* register_msg = nullptr;
	_register_id = eXosip_register_build_initial_register(
		_sip_context, _from_uri.c_str(), _proxy_uri.c_str(), _contact_url.c_str(), 3600, &register_msg);
	if (register_msg == nullptr) {
		LOG(ERROR) << "eXosip_register_build_initial_register failed";
	}

	eXosip_lock(_sip_context);
	auto ret = eXosip_register_send_register(_sip_context, _register_id, register_msg);
	eXosip_unlock(_sip_context);

	if (ret == OSIP_SUCCESS) {
		LOG(INFO) << "����ע����Ϣ";
		return true;
	}
	else {
		LOG(ERROR) << "����ע����Ϣʧ��";
		return false;
	}
}

/// @brief ֹͣsip����
/// @return 
bool SipDevice::stop_sip_client() {
	_is_running = false;

	//�˳�ʱ��ֹͣ������������
	std::scoped_lock<std::mutex> g(_session_mutex);
	for (auto&& [_, session] : _session_map)
	{
		HttpClient::GetInstance()->StopSendRtp(session);
	}

	if (_sip_thread) {
		_sip_thread->join();
		_sip_thread = nullptr;
	}

	if (_sip_context) {
		eXosip_quit(_sip_context);
		_sip_context = nullptr;
	}

	_register_success = false;
	_is_heartbeat_running = false;

	if (_subscription_thread)
	{
		_subscription_condition.notify_one();
		_subscription_thread->join();
		_subscription_thread = nullptr;
	}

	if (_heartbeat_thread) {
		_heartbeat_condition.notify_one();
		_heartbeat_thread->join();
		_heartbeat_thread = nullptr;
	}

	return true;
}

/// @brief ע��
/// @return 
bool SipDevice::log_out() {
	_is_heartbeat_running = false;

	if (_subscription_thread)
	{
		_subscription_condition.notify_one();
		_subscription_thread->join();
		_subscription_thread = nullptr;
	}

	if (_heartbeat_thread) {
		_heartbeat_condition.notify_one();
		_heartbeat_thread->join();
		_heartbeat_thread = nullptr;
	}
	_logout = true;

	LOG(INFO) << "����ע������";
	osip_message_t* register_msg = nullptr;
	auto ret = eXosip_register_build_register(_sip_context, _register_id, 0, &register_msg);
	if (ret != OSIP_SUCCESS) {
		LOG(ERROR) << "eXosip_register_build_initial_register failed";
		return false;
	}

	osip_contact_t* contact = nullptr;
	osip_message_get_contact(register_msg, 0, &contact);

	auto info = fmt::format("{};expires=0", _contact_url);
	osip_list_remove(&register_msg->contacts, 0);
	osip_message_set_contact(register_msg, info.c_str());
	osip_message_set_header(register_msg, "Logout-Reason", "logout");

	eXosip_lock(_sip_context);
	ret = eXosip_register_send_register(_sip_context, _register_id, register_msg);
	eXosip_clear_authentication_info(_sip_context);
	eXosip_unlock(_sip_context);

	return true;
}

/// @brief ��ѯ��������
void SipDevice::on_response() {

	_event_processor_map = {
		//��ѯ���Ŀ¼��ѯ��¼���ļ���ѯ���豸�����ȵȣ�
		{EXOSIP_MESSAGE_NEW,CALLBACK_TEMPLATE(on_message_new)},
		//ע��ɹ�
		{EXOSIP_REGISTRATION_SUCCESS,CALLBACK_TEMPLATE(on_registration_success)},
		//ע��ʧ��
		{EXOSIP_REGISTRATION_FAILURE,CALLBACK_TEMPLATE(on_registration_failure)},
		//�����ɾ�����豸�󣬷���������յ��˻ظ�
		{EXOSIP_MESSAGE_REQUESTFAILURE,CALLBACK_TEMPLATE(on_message_request_failure)},
		//��ʼ����
		{EXOSIP_CALL_ACK,CALLBACK_TEMPLATE(on_call_ack)},
		//�ر�����
		{EXOSIP_CALL_CLOSED,CALLBACK_TEMPLATE(on_call_closed)},
		//����Ự
		{EXOSIP_CALL_INVITE,CALLBACK_TEMPLATE(on_call_invite)},
		//��������(��ͣ����������)
		{EXOSIP_CALL_MESSAGE_NEW,CALLBACK_TEMPLATE(on_call_message_new)},
		//��Ϣ����( �ƶ��豸λ����Ϣ���� )
		{EXOSIP_IN_SUBSCRIPTION_NEW, CALLBACK_TEMPLATE(on_in_subscription_new)}
	};

	eXosip_event_t* event = nullptr;
	while (_is_running) {
		event = eXosip_event_wait(_sip_context, 0, 50);
		if (event) {
			LOG(INFO) << "------------- Receive: " << magic_enum::enum_name<eXosip_event_type>(event->type);
		}

		if (!_logout) {
			eXosip_lock(_sip_context);
			eXosip_automatic_action(_sip_context);
			eXosip_unlock(_sip_context);
		}

		if (event == nullptr)
			continue;

		auto proc = _event_processor_map.find(event->type);
		if (proc != _event_processor_map.end()) {
			_event_processor_map[event->type](event);
		}

		if (event != nullptr) {
			eXosip_event_free(event);
			event = nullptr;
		}
	}
}

/// @brief ע��ʧ��
/// @param event 
void SipDevice::on_registration_failure(eXosip_event_t* event) {
	LOG(ERROR) << "ע��ʧ��";
	_register_success = false;
	_is_heartbeat_running = false;
	if (event->response == nullptr) {
		return;
	}
	LOG(ERROR) << "ע��ʧ��: " << event->response->status_code;
	_is_heartbeat_running = false;
	if (_heartbeat_thread) {
		_heartbeat_thread->join();
		_heartbeat_thread = nullptr;
	}

	if (event->response->status_code == 401 || event->response->status_code == 403) {
		osip_www_authenticate_t* www_authenticate_header = nullptr;
		osip_message_get_www_authenticate(event->response, 0, &www_authenticate_header);
		if (www_authenticate_header)
		{
			_sip_server_domain = osip_strdup_without_quote(www_authenticate_header->realm);
			eXosip_add_authentication_info(
				_sip_context, this->ID.c_str(), this->ID.c_str(), _sip_server_info->Password.c_str(), "md5",
				www_authenticate_header->realm);
		}
	}
}

/// @brief ע��ɹ�
/// @param event 
void SipDevice::on_registration_success(eXosip_event_t* event) {
	LOG(INFO) << "ע��ɹ�";

	if (!_logout) {
		_register_success = true;
		_is_heartbeat_running = true;
		if (_heartbeat_thread == nullptr) {
			_heartbeat_thread = std::make_shared<std::thread>(&SipDevice::heartbeat_task, this);
		}
	}
}

/// @brief �յ���Ϣ��������Ҫ�в�ѯCatalog��DeviceInfo����
/// @param event 
void SipDevice::on_message_new(eXosip_event_t* event) {
	if (MSG_IS_MESSAGE(event->request)) {
		osip_body_t* body = nullptr;
		osip_message_get_body(event->request, 0, &body);
		if (body != nullptr) {
			LOG(INFO) << "request -----> \n" << body->body;
			LOG(INFO) << "------------------------------------";
		}
		send_response_ok(event);

		pugi::xml_document doc;
		auto ret = doc.load_string(body->body);
		if (ret.status != pugi::status_ok) {
			LOG(ERROR) << "load request xml failed";
			return;
		}

		auto root = doc.first_child();
		std::string root_name = root.name();
		std::string response_body = "";

		if (root_name == "Query") {
			auto cmd_node = root.child("CmdType");
			if (cmd_node.empty())
				return;
			std::string cmd = cmd_node.child_value();
			auto sn_node = root.child("SN");
			std::string sn = sn_node.child_value();
			LOG(INFO) << "SN: " << sn;
			//Ŀ¼��ѯ������channels��Ϣ
			if (cmd == "Catalog") {
				response_body = generate_catalog_xml(sn);
			}
			//�豸��Ϣ��ѯ�������豸��Ϣ,��Ҫ�����豸id���豸���ƣ����ң��汾��channel������
			else if (cmd == "DeviceInfo") {
				auto text =
					R"( <?xml version="1.0" encoding="GB2312"?>
						<Response>
							<CmdType>DeviceInfo</CmdType>
							<SN>{}</SN>
							<DeviceID>{}</DeviceID>

							<Result>OK</Result>
							<DeviceName>{}</DeviceName>
							<Manufacturer>{}</Manufacturer>

							<Model>SipServer</Model>
							<Firmware>V1.0.0</Firmware>
							<Channel>{}</Channel>
						</Response>
						)"s;

				response_body = fmt::format(text, sn, this->ID, this->Name, this->Manufacturer, this->Channels.size());
			}
			//��������
			else if (cmd == "ConfigDownload") {
				auto type_node = root.child("ConfigType");
				if (type_node.empty())
					return;
				std::string type = type_node.child_value();
				if (type == "BasicParam")
				{
					auto text =
						R"( <?xml version="1.0" encoding="GB2312"?>
							<Response>
								<CmdType>ConfigDownload</CmdType>
								<SN>{}</SN>
								<DeviceID>{}</DeviceID>

								<Result>OK</Result>
								<BasicParam>
									<Name>{}</Name>
									<DeviceID>{}</DeviceID>
									<SIPServerID>{}</SIPServerID>
									<SIPServerIP>{}</SIPServerIP>
									<SIPServerPort>{}</SIPServerPort>
									<DomainName>{}</DomainName>
									<Expiration>3600</Expiration>
									<Password>{}</Password>
									<HeartBeatInterval>{}</HeartBeatInterval>
									<HeartBeatCount>60</HeartBeatCount>
								</BasicParam>
							</Response>
							)"s;

					response_body = fmt::format(text, sn, this->ID, this->Name, this->ID, _sip_server_info->ID, _sip_server_info->IP,
						_sip_server_info->Port, _sip_server_domain, _sip_server_info->Password, this->HeartbeatInterval);
				}

				if (type == "VideoParamOpt")
				{
					auto text =
						R"( <?xml version="1.0" encoding="GB2312"?>
							<Response>
								<CmdType>ConfigDownload</CmdType>
								<SN>{}</SN>
								<DeviceID>{}</DeviceID>
								<Result>OK</Result>
								<VideoParamOpt>
									<DownloadSpeed>1</DownloadSpeed>
									<Resolution>5/6</Resolution>
								</VideoParamOpt>
							</Response>
							)"s;
					response_body = fmt::format(text, sn, this->ID);
				}
			}
			//¼���ļ���ѯ
			else if (cmd == "RecordInfo")
			{
				std::string device_id = root.child("DeviceID").child_value();
				std::string start_time = root.child("StartTime").child_value();
				std::string end_time = root.child("EndTime").child_value();

				if (event->request && event->request->req_uri && event->request->req_uri->username)
				{
					auto record_video_channel_id = event->request->req_uri->username;
					LOG(INFO) << "find record info -----> \n" << record_video_channel_id;
					std::shared_ptr<ChannelInfo> channel_info = nullptr;
					channel_info = find_channel(record_video_channel_id);
					if (channel_info != nullptr)
					{
						// ����ý���������������
						std::string response;
						auto ret = HttpClient::GetInstance()->GetMp4RecordInfo(channel_info->Stream,
							start_time, end_time, response);

						auto text =
							R"( <?xml version="1.0" encoding="GB2312"?>
									<Response>
									<CmdType>RecordInfo</CmdType>
									<SN>{}</SN>
									<DeviceID>{}</DeviceID>
									<Name>{}</Name>
									<SumNum>{}</SumNum>
									<RecordList Num="{}">
									</RecordList>
								</Response>
							)"s;

						try
						{
							pugi::xml_document doc;
							nlohmann::json json_data = nlohmann::json::parse(response);
							size_t record_size = json_data["data"]["fileInfo"].size();
							response_body = fmt::format(text, sn, this->ID, record_video_channel_id, record_size, record_size);

							auto doc_ret = doc.load_string(response_body.c_str());
							if (doc_ret.status != pugi::status_ok)
							{
								return;
							}

							auto record_list = doc.child("Response").child("RecordList");

							for (const auto& file_item : json_data["data"]["fileInfo"])
							{
								std::string file_path = std::filesystem::path(file_item["file_path"].get<std::string>()).filename().u8string();

								std::string str_begin_time = toolkit::CFileTime(file_item["begin_time"]).UTCToLocal().FormatTZ();
								std::string str_end_time = toolkit::CFileTime(file_item["stop_time"]).UTCToLocal().FormatTZ();;

								auto node = record_list.append_child("Item");
								node.append_child("DeviceID").text().set(this->ID.c_str());
								node.append_child("Name").text().set(record_video_channel_id);
								node.append_child("FilePath").text().set(file_path.c_str());
								node.append_child("StartTime").text().set(str_begin_time.c_str());
								node.append_child("EndTime").text().set(str_end_time.c_str());
								node.append_child("Address").text().set("");
								node.append_child("Secrecy").text().set("0");
								node.append_child("Type").text().set("time");
							}

							std::stringstream ss;
							doc.save(ss);
							response_body = ss.str();
						}
						catch (const std::exception& ex)
						{
							response_body = fmt::format(text, sn, this->ID, record_video_channel_id, 0, 0);
						}
					}
				}
			}

			if (!response_body.empty())
			{
				auto body = format_xml(response_body);

				osip_message_t* request = nullptr;
				auto ret = eXosip_message_build_request(
					_sip_context, &request, "MESSAGE", _proxy_uri.c_str(), _from_uri.c_str(), nullptr);
				if (ret != OSIP_SUCCESS) {
					LOG(ERROR) << "eXosip_message_build_request failed";
					return;
				}

				osip_message_set_content_type(request, "Application/MANSCDP+xml");
				osip_message_set_body(request, body.c_str(), body.length());

				eXosip_lock(_sip_context);
				eXosip_message_send_request(_sip_context, request);
				eXosip_unlock(_sip_context);
			}
		}
	}
	else if (MSG_IS_BYE(event->request)) {
		LOG(INFO) << "Receive Bye";
	}
}

/// @brief �����ɾ���豸�󣬿��ܻ��յ�����Ϣ����ʱ���·���ע����Ϣ
/// @param event 
void SipDevice::on_message_request_failure(eXosip_event_t* event) {
	osip_message_t* register_msg = nullptr;
	auto ret = eXosip_register_build_register(_sip_context, _register_id, 3600, &register_msg);
	if (register_msg == nullptr) {
		LOG(ERROR) << "eXosip_register_build_initial_register failed";
	}

	_is_running = true;

	eXosip_lock(_sip_context);
	ret = eXosip_register_send_register(_sip_context, _register_id, register_msg);
	eXosip_unlock(_sip_context);

	if (ret == OSIP_SUCCESS) {
		LOG(INFO) << "����ע����Ϣ";
	}
}

/// @brief ��ʼ����
/// @param event 
void SipDevice::on_call_ack(eXosip_event_t* event) {
	LOG(INFO) << "���յ� ACK����ʼ����";
	std::cout << "ack: did: " << event->did << "\n";

	std::scoped_lock<std::mutex> g(_session_mutex);
	if (_session_map.find(event->did) != _session_map.end())
	{
		auto session = _session_map[event->did];
		if (session->Playback)
		{
			// ����ý����������ͻط����󣬿�ʼ��ָ����ַ����RTP����
			auto ret = HttpClient::GetInstance()->StartSendPlaybackRtp(
				session->Channel, session->SSRC, session->TargetIP, session->TargetPort,
				session->LocalPort, session->StartTime, session->EndTime, session->UseTcp);
		}
		else
		{
			// TODO  ����ý��������������󣬿�ʼ��ָ����ַ����RTP����
			auto ret = HttpClient::GetInstance()->StartSendRtp(
				session->Channel, session->SSRC, session->TargetIP, session->TargetPort, session->LocalPort,
				session->UseTcp);
		}
	}
}

/// @brief ֹͣ����
/// @param event 
void SipDevice::on_call_closed(eXosip_event_t* event) {
	LOG(INFO) << "���յ� BYE����������";
	std::cout << "close: did: " << event->did << "\n";

	std::scoped_lock<std::mutex> g(_session_mutex);
	if (_session_map.find(event->did) != _session_map.end())
	{
		auto session = _session_map[event->did];
		// TODO  ����ý���������������ֹͣ����RTP����
		auto ret = HttpClient::GetInstance()->StopSendRtp(session);
		//ɾ���˻Ự
		_session_map.erase(event->did);
	}
}

/// @brief ����Ự������sdp��Ϣ
/// @param event 
void SipDevice::on_call_invite(eXosip_event_t* event) {
	LOG(INFO) << "���յ�INVITE";
	osip_body_t* sdp_body = nullptr;
	osip_message_get_body(event->request, 0, &sdp_body);
	if (sdp_body != nullptr) {
		LOG(INFO) << "request -----> \n" << sdp_body->body;
		LOG(INFO) << "------------------------------------";
	}
	else {
		LOG(ERROR) << "SDP Error";
		return;
	}

	std::cout << "invite: did: " << event->did << "\n";

	sdp_message_t* sdp = NULL;
	if (OSIP_SUCCESS != sdp_message_init(&sdp)) {
		LOG(ERROR) << "sdp_message_init failed";
		return;
	}

	std::string invite_video_channel_id = "";
	std::shared_ptr<ChannelInfo> channel_info = nullptr;
	if (event->request && event->request->req_uri && event->request->req_uri->username) {
		invite_video_channel_id = event->request->req_uri->username;
		LOG(INFO) << "INVITE: " << invite_video_channel_id;
		channel_info = find_channel(invite_video_channel_id);
	}
	else
	{
		LOG(ERROR) << "event->request->req_uri����";
		return;
	}

	if (channel_info == nullptr) {
		LOG(ERROR) << "δ�ҵ���ӦChannel: " << invite_video_channel_id;
		return;
	}


	auto ret = sdp_message_parse(sdp, sdp_body->body);
	std::string ssrc = "";
	{
		std::string text = sdp_body->body;
		ssrc = parse_ssrc(text);
		if (ssrc.empty())
		{
			LOG(ERROR) << "δ�ҵ�SSRC";
			return;
		}
	}
	sdp_connection_t* connect = eXosip_get_video_connection(sdp);
	sdp_media_t* media = eXosip_get_video_media(sdp);

	//����һ��Session�ṹ�����ڱ���DialogID���Ͷ�Ӧ��Channel��Ϣ
	auto session = std::make_shared<SessionInfo>();
	session->DialogID = event->did;
	session->TargetIP = connect->c_addr;
	session->TargetPort = std::stoi(media->m_port);
	auto protocol = media->m_proto;
	session->UseTcp = strstr(protocol, "TCP");
	session->LocalPort = NetHelper::FindAvailablePort(get_port());
	session->SSRC = ssrc;
	session->ID = invite_video_channel_id;
	session->Channel = channel_info;

	if (strstr(sdp_body->body, "s=Playback"))
	{
		session->Playback = true;
		std::string text = sdp_body->body;
		std::string start_time;
		std::string end_time;
		parse_time(text, start_time, end_time);
		session->StartTime = start_time;
		session->EndTime = end_time;
	}
	else
	{
		session->Playback = false;
	}

	LOG(INFO) << "Session:\n" << session->to_string();
	{
		std::scoped_lock<std::mutex> g(_session_mutex);
		_session_map.insert({ event->did, session });
	}

	std::stringstream ss;
	ss << "v=0\r\n";
	ss << "o={} 0 0 IN IP4 {}\r\n";
	ss << "s={}\r\n";
	ss << "c=IN IP4 {}\r\n";
	ss << "t=0 0\r\n";
	ss << "m=video {} {} 96\r\n";
	ss << "a=sendonly\r\n";
	ss << "a=rtpmap:96 PS/90000\r\n";
	ss << "y={}\r\n";

	auto info = fmt::format(
		ss.str(), invite_video_channel_id, this->IP, session->Playback ? "Playback" : "Play", this->IP,
		session->LocalPort, session->UseTcp ? "TCP/RTP/AVP" : "RTP/AVP", ssrc);

	osip_message_t* message = event->request;
	ret = eXosip_call_build_answer(_sip_context, event->tid, 200, &message);
	if (ret != OSIP_SUCCESS) {
		LOG(ERROR) << "eXosip_call_build_answer failed";
		return;
	}
	osip_message_set_content_type(message, "APPLICATION/SDP");
	osip_message_set_body(message, info.c_str(), info.length());
	eXosip_call_send_answer(_sip_context, event->tid, 200, message);

	LOG(INFO) << "SDP Response: " << info;
}

/// @brief ���سɹ���Ϣ
/// @param event 
void SipDevice::send_response_ok(eXosip_event_t* event) {
	osip_message_t* message = event->request;
	eXosip_message_build_answer(_sip_context, event->tid, 200, &message);

	eXosip_lock(_sip_context);
	eXosip_message_send_answer(_sip_context, event->tid, 200, message);
	eXosip_unlock(_sip_context);
}

/// @brief ����
void SipDevice::heartbeat_task() {
	while (_is_running && _register_success && _is_heartbeat_running) {
		auto text =
			R"(<?xml version="1.0" encoding="GB2312"?>
				<Notify>
					<CmdType>Keepalive</CmdType>
					<SN>{}</SN>
					<DeviceID>{}</DeviceID>
					<Status>OK</Status>
				</Notify>
				)"s;
		auto info = fmt::format(text, get_sn(), this->ID);

		osip_message_t* request = nullptr;
		auto ret = eXosip_message_build_request(
			_sip_context, &request, "MESSAGE", _proxy_uri.c_str(), _from_uri.c_str(), nullptr);
		if (ret != OSIP_SUCCESS) {
			LOG(ERROR) << "eXosip_message_build_request failed";
			return;
		}

		osip_message_set_content_type(request, "Application/MANSCDP+xml");
		osip_message_set_body(request, info.c_str(), info.length());

		eXosip_lock(_sip_context);
		eXosip_message_send_request(_sip_context, request);
		eXosip_unlock(_sip_context);

		LOG(INFO) << "Heartbeat";

		std::unique_lock<std::mutex> lck(_heartbeat_mutex);
		_heartbeat_condition.wait_for(lck, std::chrono::seconds(this->HeartbeatInterval));
	}
}

void SipDevice::mobile_position_task()
{
	while (_is_running && _register_success && _subscription_dialog_id > 0) {
		osip_message_t* notify_message = NULL;
		if (OSIP_SUCCESS != eXosip_insubscription_build_notify(_sip_context, _subscription_dialog_id, EXOSIP_SUBCRSTATE_PENDING, EXOSIP_NOTIFY_PENDING, &notify_message)) {
			LOG(ERROR) << "eXosip_insubscription_build_notify error";
		}
		else
		{
			std::stringstream ss;
			ss << "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n";
			ss << "<Notify>\r\n";
			ss << "<DeviceID>" << this->ID << "</DeviceID>\r\n";
			ss << "<CmdType>MobilePosition</CmdType>\r\n";
			ss << "<SN>" << get_sn() << "</SN>\r\n";
			ss << "<Time>" << "</Time>\r\n";
			ss << "<Longitude>" << "103.22925" << "</Longitude>\r\n";
			ss << "<Latitude>" << "31.23354 " << "</Latitude>\r\n";
			ss << "<Speed>0.0</Speed>\r\n";
			ss << "<Direction>0.0</Direction>\r\n";
			ss << "<Altitude>500</Altitude>\r\n";
			ss << "</Notify>\r\n";

			auto text = ss.str();
			osip_message_set_content_type(notify_message, "Application/MANSCDP+xml");
			osip_message_set_body(notify_message, text.c_str(), text.length());
			eXosip_insubscription_send_request(_sip_context, _subscription_dialog_id, notify_message);

			LOG(INFO) << "Insubscription";

			std::unique_lock<std::mutex> lck(_subscription_mutex);
			_subscription_condition.wait_for(lck, std::chrono::seconds(this->HeartbeatInterval));
		}
	}
}

/// @brief ����Ŀ¼��Ϣ
/// @param sn 
/// @return 
std::string SipDevice::generate_catalog_xml(const std::string& sn) {
	auto text =
		R"( <?xml version="1.0" encoding="GB2312"?>
			<Response>
				<CmdType>Catalog</CmdType>
				<SN>{}</SN>
				<DeviceID>{}</DeviceID>
				<SumNum>{}</SumNum>
				<DeviceList Num="{}">
				</DeviceList>
			</Response>
			)"s;

	auto xml = fmt::format(text, sn, this->ID, this->Channels.size(), this->Channels.size());
	pugi::xml_document doc;
	auto ret = doc.load_string(xml.c_str());
	if (ret.status != pugi::status_ok) {
		return "";
	}

	auto device = doc.child("Response").child("DeviceList");
	for (auto&& channel : this->Channels) {
		auto node = device.append_child("Item");
		node.append_child("DeviceID").text().set(channel->ID.c_str());
		node.append_child("Name").text().set(channel->Name.c_str());
		node.append_child("Manufacturer").text().set(this->Manufacturer.c_str());
		node.append_child("Model").text().set(this->Name.c_str());
		node.append_child("Status").text().set("ON");
	}

	std::stringstream ss;
	doc.save(ss);

	return ss.str();
}

std::string SipDevice::parse_ssrc(std::string text) {
	text = nbase::StringTrim(text);
	auto tokens = nbase::StringTokenize(text.c_str(), "\n");
	for (auto&& token : tokens)
	{
		if (strstr(token.c_str(), "y=0") || strstr(token.c_str(), "y=1")) {
			auto result = token.substr(2);
			LOG(INFO) << "---Y: " << result;
			return result;
		}
	}

	return std::string();
}

bool SipDevice::parse_time(std::string text, std::string& start_time, std::string& end_time)
{
	text = nbase::StringTrim(text);
	auto tokens = nbase::StringTokenize(text.c_str(), "\n");
	for (auto&& token : tokens)
	{
		if (strstr(token.c_str(), "t="))
		{
			start_time = token.substr(2, 11);
			end_time = token.substr(12);

			nbase::StringTrim(start_time);
			nbase::StringTrim(end_time);
			return true;
		}
	}
	return false;
}


std::shared_ptr<ChannelInfo> SipDevice::find_channel(std::string id) {
	std::shared_ptr<ChannelInfo> channel_info = nullptr;
	for (auto&& channel : this->Channels) {
		if (channel->ID.compare(id) == 0) {
			channel_info = channel;
			break;
		}
	}

	return channel_info;
}


void SipDevice::on_in_subscription_new(eXosip_event_t* event)
{
	LOG(INFO) << "on_in_subscription_new...";

	osip_message_t* answer = NULL;
	if (OSIP_SUCCESS == eXosip_insubscription_build_answer(_sip_context, event->tid, 200, &answer)) {
		eXosip_insubscription_send_answer(_sip_context, event->tid, 200, answer);
		_subscription_dialog_id = event->did;

		if (!_logout) {
			if (_subscription_thread == nullptr) {
				_subscription_thread = std::make_shared<std::thread>(&SipDevice::mobile_position_task, this);
			}
		}
	}
}


/// @brief ��Ӧ���ſ�������
/// @param event 
/// @return
void SipDevice::on_call_message_new(eXosip_event_t* event)
{
	if (MSG_IS_INFO(event->request))
	{
		std::cout << "msg: did: " << event->did << "\n";

		if (_session_map.find(event->did) != _session_map.end()) {
			auto session = _session_map[event->did];
			if (session->Playback)
			{
				osip_body_t* body = nullptr;
				osip_message_get_body(event->request, 0, &body);
				if (body != nullptr)
				{
					LOG(INFO) << "request -----> \n" << body->body;
					if (strstr(body->body, "PAUSE"))
					{
						// ����ý�������������ͣ����
						auto ret = HttpClient::GetInstance()->SetPause("record", session->SSRC, true);
					}
					else if (strstr(body->body, "PLAY") && strstr(body->body, "Range"))
					{
						auto ret = HttpClient::GetInstance()->SetPause("record", session->SSRC, false);
					}
					else if (strstr(body->body, "PLAY") && strstr(body->body, "Scale"))
					{
						auto tokens = nbase::StringTokenize(body->body, "\n");
						for (auto&& token : tokens)
						{
							if (strstr(token.c_str(), "Scale"))
							{
								auto text = token.substr(6);
								nbase::StringTrim(text);

								auto speed = std::stof(text);
								auto ret = HttpClient::GetInstance()->SetSpeed("record", session->SSRC, speed);
							}
						}
					}
				}
			}
		}
		send_response_ok(event);
	}
}


///
bool SipDevice::send_notify(std::shared_ptr<SessionInfo> session)
{
	auto text =
		R"(<?xml version="1.0" encoding="GB2312"?>
				<Notify>
					<CmdType>MediaStatus</CmdType>
					<SN>{}</SN>
					<DeviceID>{}</DeviceID>
					<NotifyType>121</NotifyType>
				</Notify>
				)"s;
	auto info = fmt::format(text, get_sn(), this->ID);

	osip_message_t* request = nullptr;
	auto ret = eXosip_message_build_request(
		_sip_context, &request, "MESSAGE", _proxy_uri.c_str(), _from_uri.c_str(), nullptr);
	if (ret != OSIP_SUCCESS) {
		LOG(ERROR) << "eXosip_message_build_request failed";
		return false;
	}

	osip_message_set_content_type(request, "Application/MANSCDP+xml");
	osip_message_set_body(request, info.c_str(), info.length());

	eXosip_lock(_sip_context);
	eXosip_message_send_request(_sip_context, request);
	eXosip_unlock(_sip_context);

	return true;
}


std::string SipDevice::format_xml(std::string& xml)
{
	pugi::xml_document doc;
	auto ret = doc.load_string(xml.c_str());
	if (ret.status != pugi::status_ok) {
		return xml;
	}

	std::stringstream ss;
	doc.save(ss);

	return ss.str();
}

