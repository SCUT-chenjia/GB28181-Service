﻿#include "pch.h"
#include "EventHandler.h"
#include "ConfigManager.h"
#include "HttpDigest.h"
#include "ConfigManager.h"
#include "DeviceManager.h"
#include "SipRequest.h"
#include "XmlParser.h"
#include "StreamManager.h"
#include "RequestPool.h"
#include "DbManager.h"

bool BaseEventHandler::Handle(const SipEvent::Ptr& e, pugi::xml_document& doc)
{
	return false;
}

int BaseEventHandler::SendResponse(const char* uname, eXosip_t* excontext, int tid, int status)
{
	osip_message_t* answer = nullptr;
	eXosip_lock(excontext);
	eXosip_message_build_answer(excontext, tid, status, &answer);
	int ret = eXosip_message_send_answer(excontext, tid, status, nullptr);
	eXosip_unlock(excontext);
	return ret;
}

int BaseEventHandler::SendResponseAndGetAddress(const char* uname, eXosip_t* excontext, int tid, int status, std::string& address, uint16_t& port)
{
	osip_message_t* answer = nullptr;
	eXosip_lock(excontext);
	eXosip_message_build_answer(excontext, tid, status, &answer);

	char addr[50]{};

	osip_via_t* via = nullptr;
	osip_message_get_via(answer, 0, &via);

	if (via && via->host)
	{
		osip_generic_param_t* param = nullptr;
		osip_via_param_get_byname(via, (char*)"received", &param);
		if (param && param->gvalue)
		{
			strcpy(addr, param->gvalue);
		}
		else
		{
			strcpy(addr, via->host);
		}
		address = std::string(addr);

		osip_via_param_get_byname(via, (char*)"rport", &param);
		if (param && param->gvalue)
		{
			port = std::stoi(param->gvalue);
		}
	}

	int ret = eXosip_message_send_answer(excontext, tid, status, nullptr);
	eXosip_unlock(excontext);
	return ret;
}

int BaseEventHandler::SendCallAck(eXosip_t* excontext, int did)
{
	osip_message_t* ack = nullptr;
	eXosip_lock(excontext);
	eXosip_call_build_ack(excontext, did, &ack);
	int ret = eXosip_call_send_ack(excontext, did, ack);
	eXosip_unlock(excontext);
	return ret;
}

int BaseEventHandler::GetStatusCodeFromResponse(osip_message_t* response)
{
	return response != nullptr ? response->status_code : -1;
}

std::string BaseEventHandler::GetMsgIDFromRequest(osip_message_t* request)
{
	osip_generic_param_t* tag = nullptr;
	//osip_to_get_tag(request->from, &tag);
	osip_uri_param_get_byname(&request->from->gen_params, (char*)"tag", &tag);

	if (nullptr == tag || nullptr == tag->gvalue) {
		return "";
	}
	return tag->gvalue;
}

int RegisterHandler::HandleIncomingRequest(const SipEvent::Ptr& e)
{
	osip_authorization_t* authorization = nullptr;
	osip_message_get_authorization(e->exosip_event->request, 0, &authorization);

	if (authorization && authorization->username)
	{
		char* method = nullptr;
		char* username = nullptr;
		char* uri = nullptr;
		char* response = nullptr;

		//osip_contact_t* contact = nullptr;
		//osip_message_get_contact(e->exosip_event->request, 0, &contact);
		//if (contact == nullptr)
		//{
		//	SendResponse(username, e->exosip_context, e->exosip_event->tid, SIP_BAD_REQUEST);
		//	SPDLOG_WARN("Device Regist Failed, contact = null");
		//	return -1;
		//}
		auto config = ConfigManager::GetInstance()->GetSipServerInfo();

		method = e->exosip_event->request->sip_method;

		auto sip_id = e->exosip_event->request->req_uri->username;
		if (strcmp(sip_id, config->ID.c_str()) != 0)
		{
			SendResponse(username, e->exosip_context, e->exosip_event->tid, SIP_BAD_REQUEST);
			SPDLOG_WARN("Device Regist Failed, sip id doesn't match");
			return -1;
		}

#define SIP_STRDUP(field)  if(authorization->field) (field) = osip_strdup_without_quote(authorization->field);

		SIP_STRDUP(uri);
		SIP_STRDUP(username);
		SIP_STRDUP(response);

		HASHHEX HA1, calc_response;
		DigestCalcHA1("REGISTER", username, config->Realm.c_str(), config->Password.c_str(),
			config->Nonce.c_str(), NULL, HA1);
		DigestCalcResponse(HA1, config->Nonce.c_str(), NULL, NULL, NULL, 0, method, uri, NULL,
			calc_response);
		SPDLOG_INFO("MD5: {}", calc_response);

		//std::string client_host = strdup(contact->url->host);
		//auto client_port = strdup(contact->url->port);
		auto client_device_id = username;

		if (0 == memcmp(calc_response, response, HASHHEXLEN))
		{
			std::string client_host;
			uint16_t port;

			SendResponseAndGetAddress(username, e->exosip_context, e->exosip_event->tid, SIP_OK, client_host, port);
			SPDLOG_INFO("Device Registration Success, Address: {}:{}    ID:{}", client_host, port, client_device_id);

			std::string client_port = std::to_string(port);
			//TOOD:
			bool find = false;
			auto device = DeviceManager::GetInstance()->GetDevice(client_device_id);
			if (device == nullptr)
			{
				device = std::make_shared<Device>(client_device_id, client_host, client_port);
				device->SetStreamIP(config->ExternIP);
				DeviceManager::GetInstance()->AddDevice(device);
			}
			else
			{
				//如果设备已经存在的话，就只更新在线状态和注册时间
				find = true;
				if (device->GetIP() != client_host || device->GetPort() != client_port)
				{
					SPDLOG_WARN("设备地址变化:  {}  {}->{}", client_device_id, device->GetIP(), client_host);
					device->SetIP(client_host);
					device->SetPort(client_port);
				}
			}

			device->SetRegistered(true);
			device->SetStatus(1);
			device->UpdateRegistTime();
			device->UpdateLastTime();

			//将设备和sip context关联，用于后期判断处理方便。
			device->exosip_context = e->exosip_context;

			DbManager::GetInstance()->AddOrUpdateDevice(device, find);

			{
				auto request = std::make_shared<DeviceInfoRequest>(e->exosip_context, device);
				request->SendMessage();
			}

			{
				auto request = std::make_shared<CatalogRequest>(e->exosip_context, device);
				request->SendMessage();
			}
		}
		else
		{
			SendResponse(username, e->exosip_context, e->exosip_event->tid, SIP_UNAUTHORIZED);
			SPDLOG_INFO("Device Regist Failed, ID: {}", client_device_id);

			//TODO:
			DeviceManager::GetInstance()->RemoveDevice(client_device_id);
		}
		osip_free(uri);
		osip_free(username);
		osip_free(response);
	}
	else
	{
		_response_register_401unauthorized(e);
	}

	return 0;
}

void RegisterHandler::_response_register_401unauthorized(const SipEvent::Ptr& e)
{
	osip_www_authenticate_t* www_authenticate_header = nullptr;
	osip_www_authenticate_init(&www_authenticate_header);

	char* dest = nullptr;
	osip_message_t* response = nullptr;

	auto config = ConfigManager::GetInstance()->GetSipServerInfo();

	osip_www_authenticate_set_auth_type(www_authenticate_header, osip_strdup("Digest"));
	osip_www_authenticate_set_realm(www_authenticate_header, osip_enquote(config->Realm.c_str()));
	osip_www_authenticate_set_nonce(www_authenticate_header, osip_enquote(config->Nonce.c_str()));
	osip_www_authenticate_to_str(www_authenticate_header, &dest);
	int ret = eXosip_message_build_answer(e->exosip_context, e->exosip_event->tid, SIP_UNAUTHORIZED, &response);
	if (ret == 0 && response != nullptr)
	{
		osip_message_set_www_authenticate(response, dest);
		osip_message_set_content_type(response, "Application/MANSCDP+xml");
		eXosip_lock(e->exosip_context);
		eXosip_message_send_answer(e->exosip_context, e->exosip_event->tid, SIP_UNAUTHORIZED, response);
		eXosip_unlock(e->exosip_context);
		SPDLOG_INFO("response_register_401unauthorized success");
	}
	else
	{
		SPDLOG_ERROR("response_register_401unauthorized error");
	}

	osip_www_authenticate_free(www_authenticate_header);
	osip_free(dest);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------


int MessageHandler::HandleIncomingRequest(const SipEvent::Ptr& e)
{
	auto username = e->exosip_event->request->from->url->username;
	auto host = e->exosip_event->request->from->url->host;

	osip_body_t* body = nullptr;
	osip_message_get_body(e->exosip_event->request, 0, &body);
	if (body == nullptr)
	{
		SendResponse(username, e->exosip_context, e->exosip_event->tid, SIP_BAD_REQUEST);
		return -1;
	}

	SPDLOG_INFO("============================================");
	SPDLOG_INFO(std::string(body->body));

	XmlParser parser;
	pugi::xml_document doc;
	auto ret = parser.Parse(body->body, (int)body->length, doc);
	if (!ret)
	{
		SendResponse(username, e->exosip_context, e->exosip_event->tid, SIP_BAD_REQUEST);
		return 0;
	}

	manscdp_msgbody_header_t header;
	ret = parser.ParseHeader(header, doc);
	if (!ret)
	{
		SendResponse(username, e->exosip_context, e->exosip_event->tid, SIP_BAD_REQUEST);
		return 0;
	}

	switch (header.cmd_category)
	{
	case MANSCDP_CMD_CATEGORY_CONTROL:
		break;
	case MANSCDP_CMD_CATEGORY_QUERY:
		break;
	case MANSCDP_CMD_CATEGORY_NOTIFY:
		if (header.cmd_type == MANSCDP_NOTIFY_CMD_KEEPALIVE)
		{
			HeartbeatHandler h;
			h.Handle(e, doc);
		}
		break;
	case MANSCDP_CMD_CATEGORY_RESPONSE:
		if (header.cmd_type == MANSCDP_QUERY_CMD_CATALOG)
		{
			CatalogHandler h;
			h.Handle(e, doc);
		}
		else if (header.cmd_type == MANSCDP_QUERY_CMD_DEVICE_INFO)
		{
			DeviceInfoHandler h;
			h.Handle(e, doc);
		}
		else if (header.cmd_type == MANSCDP_QUERY_CMD_PRESET_QUERY)
		{
			PresetQueryHandler h;
			h.Handle(e, doc);
		}
		else if (header.cmd_type == MANSCDP_QUERY_CMD_RECORD_INFO)
		{
			RecordQueryHandler h;
			h.Handle(e, doc);
		}

		break;
	default:
		break;
	}

	return 0;
}

int MessageHandler::HandleResponseSuccess(const SipEvent::Ptr& e)
{
	int code = GetStatusCodeFromResponse(e->exosip_event->response);
	auto id = GetMsgIDFromRequest(e->exosip_event->request);

	RequestPool::GetInstance()->HandleMessageRequest(id, code);

	return 0;
}

int MessageHandler::HandleResponseFailure(const SipEvent::Ptr& e)
{
	int code = GetStatusCodeFromResponse(e->exosip_event->response);
	auto id = GetMsgIDFromRequest(e->exosip_event->request);

	RequestPool::GetInstance()->HandleMessageRequest(id, code);
	return 0;
}

bool CatalogHandler::Handle(const SipEvent::Ptr& e, pugi::xml_document& doc)
{
	auto root = doc.first_child();

	auto device_id = root.child("DeviceID").text().as_string();
	auto device = DeviceManager::GetInstance()->GetDevice(device_id);
	if (device == nullptr)
	{
		SendResponse(device_id, e->exosip_context, e->exosip_event->tid, SIP_BAD_REQUEST);
		return false;
	}

	auto node = root.child("DeviceList");
	if (node.empty())
	{
		return false;
	}

	auto children = node.children("Item");
	for (auto&& child : children)
	{
		auto channel_id = child.child("DeviceID").text().as_string();

		bool find = false;
		std::shared_ptr<Channel> channel = nullptr;
		channel = device->GetChannel(channel_id);
		if (channel == nullptr)
		{
			channel = std::make_shared<Channel>();
			channel->SetDefaultSSRC(SSRCConfig::GetInstance()->GenerateSSRC());
			device->InsertChannel(device_id, channel_id, channel);
		}
		else
		{
			find = true;
		}

		channel->SetChannelID(child.child("DeviceID").text().as_string());
		channel->SetName(child.child("Name").text().as_string());
		channel->SetManufacturer(child.child("Manufacturer").text().as_string());
		channel->SetModel(child.child("Model").text().as_string());
		channel->SetOwner(child.child("Owner").text().as_string());
		channel->SetCivilCode(child.child("CivilCode").text().as_string());
		channel->SetAddress(child.child("Address").text().as_string());
		channel->SetParental(child.child("Parental").text().as_string());
		channel->SetParentID(child.child("ParentID").text().as_string());
		channel->SetRegisterWay(child.child("RegisterWay").text().as_string());
		channel->SetSecrety(child.child("Secrecy").text().as_string());
		channel->SetStreamNum(child.child("StreamNum").text().as_string());
		channel->SetIpAddress(child.child("IPAddress").text().as_string());
		channel->SetStatus(child.child("Status").text().as_string());

		auto info = child.child("Info");
		if (info && info.child("PTZType"))
		{
			channel->SetPtzType(info.child("PTZType").text().as_string());
		}
		if (info && info.child("DownloadSpeed"))
		{
			channel->SetDownloadSpeed(info.child("DownloadSpeed").text().as_string());
		}

		DbManager::GetInstance()->AddOrUpdateChannel(device_id, channel, find);
	}

	SendResponse(device_id, e->exosip_context, e->exosip_event->tid, SIP_OK);
	return true;
}

bool HeartbeatHandler::Handle(const SipEvent::Ptr& e, pugi::xml_document& doc)
{
	auto root = doc.first_child();

	auto device_id = root.child("DeviceID").text().as_string();
	auto device = DeviceManager::GetInstance()->GetDevice(device_id);
	if (device && device->IsRegistered())
	{
		device->UpdateLastTime();
		device->SetStatus(1);
		DbManager::GetInstance()->AddOrUpdateDevice(device, true);
		SendResponse(device_id, e->exosip_context, e->exosip_event->tid, SIP_OK);
		return true;
	}
	else
	{
		SPDLOG_WARN("Heartbeat SIP_UNAUTHORIZED");
		SendResponse(device_id, e->exosip_context, e->exosip_event->tid, SIP_UNAUTHORIZED);
	}
	return false;
}


bool DeviceInfoHandler::Handle(const SipEvent::Ptr& e, pugi::xml_document& doc)
{
	auto root = doc.first_child();

	auto device_id = root.child("DeviceID").text().as_string();
	auto device = DeviceManager::GetInstance()->GetDevice(device_id);
	if (device)
	{
		device->SetName(root.child("DeviceName").text().as_string());
		device->SetManufacturer(root.child("Manufacturer").text().as_string());
		device->SetModel(root.child("Model").text().as_string());

		SPDLOG_INFO(device->toString());
		DbManager::GetInstance()->AddOrUpdateDevice(device, true);
		SendResponse(device_id, e->exosip_context, e->exosip_event->tid, SIP_OK);
		return true;
	}
	return false;
}


int CallHandler::HandleResponseSuccess(const SipEvent::Ptr e)
{
	std::string device_id = e->exosip_event->request->to->url->username;
	std::string host = e->exosip_event->request->to->url->host;
	std::string port = e->exosip_event->request->to->url->port;

	int call_id = e->exosip_event->cid;
	int dialog_id = e->exosip_event->did;

	SPDLOG_INFO("on_exosip_call_answered DeviceID:  {}    CallID:{}    DialogID:{} ", device_id, call_id, dialog_id);

	auto device = DeviceManager::GetInstance()->GetDevice(host, port);
	if (device == nullptr)
	{
		SPDLOG_WARN("Device Not Exists: {}", device_id);
		return -1;
	}

	auto sessions = StreamManager::GetInstance()->GetStreamByType(STREAM_TYPE_GB);
	for (auto&& s : sessions)
	{
		auto session = std::dynamic_pointer_cast<CallSession>(s);

		if (session->GetCallID() == call_id)
		{
			session->SetDialogID(dialog_id);
			session->SetConnected(true);

			SendCallAck(e->exosip_context, dialog_id);
			return 0;
		}
	}
	return -1;
}

int CallHandler::on_proceeding(const SipEvent::Ptr e)
{
	std::string reqid;
	osip_generic_param_t* tag = nullptr;
	//osip_to_get_tag(e->exosip_event->request->from, &tag);
	osip_uri_param_get_byname(&e->exosip_event->request->from->gen_params, (char*)"tag", &tag);

	if (nullptr == tag || nullptr == tag->gvalue) {
		reqid = "";
	}
	reqid = (const char*)tag->gvalue;

	SPDLOG_INFO("on_exosip_call_proceeding response reqid = {}", reqid);
	return 0;
}

int CallHandler::HandleClose(const SipEvent::Ptr e)
{
	std::string device_id = e->exosip_event->request->to->url->username;

	int call_id = e->exosip_event->cid;
	int dialog_id = e->exosip_event->did;

	SPDLOG_INFO("on_exosip_call_close DeviceID: {}     CallID:{}    DialogID:{}", device_id, call_id, dialog_id);

	auto sessions = StreamManager::GetInstance()->GetStreamByType(STREAM_TYPE_GB);
	for (auto&& s : sessions)
	{
		auto session = std::dynamic_pointer_cast<CallSession>(s);

		if (session->GetCallID() == call_id)
		{
			session->SetDialogID(dialog_id);
			session->SetConnected(false);
			return 0;
		}
	}
	SPDLOG_WARN("CallID not found: {}", call_id);
	return -1;
}



bool PresetQueryHandler::Handle(const SipEvent::Ptr& e, pugi::xml_document& doc)
{
	auto root = doc.first_child();
	auto sn = root.child("SN").text().as_string();;
	auto device_id = root.child("DeviceID").text().as_string();

	auto req = RequestPool::GetInstance()->GetMessageRequestBySN(sn, DEVICE_QUERY_PRESET);
	if (req == nullptr)
	{
		SPDLOG_ERROR("PresetQueryHandler can not find request by sn: {}", sn);
		SendResponse(device_id, e->exosip_context, e->exosip_event->tid, SIP_INTERNAL_SERVER_ERROR);
		return false;
	}

	auto node = root.child("PresetList");
	auto num = node.attribute("Num").as_int();
	SPDLOG_INFO("DeviceID: {}    Preset:{}", device_id, num);

	auto request = std::dynamic_pointer_cast<PresetRequest>(req);
	auto children = node.children("Item");
	for (auto&& child : children)
	{
		auto id = child.child("PresetID").text().as_string();
		auto name = child.child("PresetName").text().as_string();

		request->InsertPreset(id, name);
	}
	request->OnRequestFinished();
	SendResponse(device_id, e->exosip_context, e->exosip_event->tid, SIP_OK);
	return true;
}

bool RecordQueryHandler::Handle(const SipEvent::Ptr& e, pugi::xml_document& doc)
{
	auto root = doc.first_child();
	auto sn = root.child("SN").text().as_string();;
	auto device_id = root.child("DeviceID").text().as_string();

	auto req = RequestPool::GetInstance()->GetMessageRequestBySN(sn, DEVICE_RECORD_QUERY);
	if (req == nullptr)
	{
		SPDLOG_ERROR("RecordQueryHandler can not find request by sn: {}", sn);
		SendResponse(device_id, e->exosip_context, e->exosip_event->tid, SIP_INTERNAL_SERVER_ERROR);
		return false;
	}

	auto sum_num = root.child("SumNum").text().as_int();
	auto request = std::dynamic_pointer_cast<RecordRequest>(req);

	auto node = root.child("RecordList");
	auto num = node.attribute("Num").as_int();
	SPDLOG_INFO("DeviceID: {}     Record:{}", device_id, num);

	auto children = node.children("Item");
	for (auto&& child : children)
	{
		auto item = std::make_shared<RecordItem>();
		item->DeviceID = child.child("DeviceID").text().as_string();
		item->Name = child.child("Name").text().as_string();
		item->FilePath = child.child("FilePath").text().as_string();
		item->Address = child.child("Address").text().as_string();
		item->StartTime = child.child("StartTime").text().as_string();
		item->EndTime = child.child("EndTime").text().as_string();
		item->Secrecy = child.child("Secrecy").text().as_int();
		item->Type = child.child("Type").text().as_string();
		item->FileSize = child.child("FileSize").text().as_ullong();
		request->InsertRecord(item);
	}

	if (request->GetRecordSize() == sum_num)
	{
		request->OnRequestFinished();
	}
	SendResponse(device_id, e->exosip_context, e->exosip_event->tid, SIP_OK);

	return true;
}
