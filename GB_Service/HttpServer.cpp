#include "pch.h"
#include "HttpServer.h"
#include "DeviceManager.h"
#include "StreamManager.h"
#include "SipRequest.h"
#include "SipServer.h"


HttpServer::HttpServer()
	:_api_blueprint("v1")
	, _hook_blueprint("index/hook")
{
	CROW_ROUTE(_app, "/")([]() {return "Hello World!"; });

	CROW_BP_ROUTE(_api_blueprint, "/")([]() {return "Hello World !"; });

	CROW_BP_ROUTE(_api_blueprint, "/devicelist")([this]()
		{
			auto devices = DeviceManager::GetInstance()->GetDeviceList();
			auto doc = nlohmann::json::array();
			for (auto&& dev : devices)
			{
				doc.push_back(dev->toJson());
			}
			return _mk_response(0, doc);
		}
	);

	CROW_BP_ROUTE(_api_blueprint, "/device/<string>/channellist")([this](std::string device_id)
		{
			auto doc = nlohmann::json::array();
			auto device = DeviceManager::GetInstance()->GetDevice(device_id);
			if (device)
			{
				auto channels = device->GetAllChannels();
				for (auto&& channel : channels)
				{
					doc.push_back(channel->toJson());
				}

				return _mk_response(0, doc);
			}
			else
			{
				return _mk_response(1, "", "device not found");
			}
		}
	);


	CROW_BP_ROUTE(_api_blueprint, "/device/<string>")([this](std::string device_id)
		{
			auto device = DeviceManager::GetInstance()->GetDevice(device_id);
			if (device)
			{
				return _mk_response(0, device->toJson());
			}
			else
			{
				return _mk_response(1, "", "device not found");
			}
		}
	);

	CROW_BP_ROUTE(_api_blueprint, "/device/<string>/channel/<string>")
		([this](std::string device_id, std::string channel_id)
		{
			auto device = DeviceManager::GetInstance()->GetDevice(device_id);
			if (device == nullptr)
			{
				return _mk_response(1, "", "device not found");
			}

			auto channel = device->GetChannel(channel_id);
			if (channel == nullptr)
			{
				return _mk_response(1, "", "channel not found");
			}

			return _mk_response(0, channel->toJson());
		}
	);

	CROW_BP_ROUTE(_api_blueprint, "/streamlist")
		([this]()
		{
			auto streams = StreamManager::GetInstance()->GetAllStream();
			auto doc = nlohmann::json::array();
			for (auto&& s : streams)
			{
				doc.push_back(s->toJson());
			}
			return _mk_response(0, doc);
		}
	);

	CROW_BP_ROUTE(_api_blueprint, "/device/<string>/channel/<string>/play")
		([this](std::string device_id, std::string channel_id)
		{
			auto device = DeviceManager::GetInstance()->GetDevice(device_id);
			if (device == nullptr)
			{
				return _mk_response(1, "", "device not found");
			}

			auto channel = device->GetChannel(channel_id);
			if (channel == nullptr)
			{
				return _mk_response(1, "", "channel not found");
			}
			auto stream_id = fmt::format("{}_{}", device_id, channel_id);

			InviteRequest::Ptr request = nullptr;
			auto stream = StreamManager::GetInstance()->GetStream(stream_id);
			if (stream)
			{
				auto session = std::dynamic_pointer_cast<CallSession>(stream);
				if (session->IsConnected())
				{
					return _mk_response(400, "", "already exists");
				}
			}

			request = std::make_shared<InviteRequest>(
				SipServer::GetInstance()->GetSipContext(), device, channel_id);

			request->SendCall();

			stream = StreamManager::GetInstance()->GetStream(stream_id);
			if (stream)
			{
				auto session = std::dynamic_pointer_cast<CallSession>(stream);
				auto ret = session->WaitForStreamReady();

				if (!ret)
				{
					return _mk_response(400, "", "timeout");
				}
			}

			return _mk_response(0, "", "ok");
		}
	);

	CROW_BP_ROUTE(_api_blueprint, "/device/<string>/channel/<string>/stop")
		([this](std::string device_id, std::string channel_id)
		{
			auto device = DeviceManager::GetInstance()->GetDevice(device_id);
			if (device == nullptr)
			{
				return _mk_response(1, "", "device not found");
			}

			auto channel = device->GetChannel(channel_id);
			if (channel == nullptr)
			{
				return _mk_response(1, "", "channel not found");
			}
			auto stream_id = fmt::format("{}_{}", device_id, channel_id);
			auto stream = StreamManager::GetInstance()->GetStream(stream_id);
			if (stream)
			{
				auto session = std::dynamic_pointer_cast<CallSession>(stream);
				if (!session->IsConnected())
				{
					return _mk_response(400, "", "not play");
				}
				else
				{
					eXosip_call_terminate(SipServer::GetInstance()->GetSipContext(),
						session->GetCallID(), session->GetDialogID());
					session->SetConnected(false);

					return _mk_response(0, "", "ok");
				}
			}
			else
			{
				return _mk_response(400, "", "not play");
			}
		}
	);


	//-------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------

	CROW_BP_ROUTE(_hook_blueprint, "/")([]() {return "Hello World !"; });


	//��������ʱ�ϱ���ȷ�Ϸ������Ƿ�����
	CROW_BP_ROUTE(_hook_blueprint, "/on_server_keepalive").methods("POST"_method)([this](const crow::request& req)
		{
			return "Hello World !";
		}
	);

	//��ע���ע��ʱ�������¼�
	CROW_BP_ROUTE(_hook_blueprint, "/on_stream_changed").methods("POST"_method)([this](const crow::request& req)
		{
			return "Hello World !";
		}
	);

	//�����˹ۿ�ʱ�¼����û�����ͨ�����¼�ѡ���Ƿ�ر����˿�����
	CROW_BP_ROUTE(_hook_blueprint, "/on_stream_none_reader").methods("POST"_method)([this](const crow::request& req)
		{
			return "Hello World !";
		}
	);

	//��δ�ҵ��¼����û������ڴ��¼�����ʱ������ȥ��������������ʵ�ְ�������
	CROW_BP_ROUTE(_hook_blueprint, "/on_stream_not_found").methods("POST"_method)([this](const crow::request& req)
		{
			return "Hello World !";
		}
	);

	//����openRtpServer �ӿڣ�rtp server ��ʱ��δ�յ�����,ִ�д�web hook
	CROW_BP_ROUTE(_hook_blueprint, "/on_rtp_server_timeout").methods("POST"_method)([this](const crow::request& req)
		{
			return "Hello World !";
		}
	);

	_app.register_blueprint(_api_blueprint);
	_app.register_blueprint(_hook_blueprint);
}


void HttpServer::Start(int port)
{
	_app.loglevel(crow::LogLevel::Critical).port(port).multithreaded().run_async();
}