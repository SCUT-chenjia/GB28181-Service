#include "pch.h"
#include "ConfigManager.h"

bool ConfigManager::LoadConfig(std::wstring filepath) {
	LOG(INFO) << "�����ļ�����...";
	pugi::xml_document doc;
	auto ret = doc.load_file(filepath.c_str(), pugi::parse_full);
	if (ret.status != pugi::status_ok) {
		LOG(ERROR) << "�����ļ�����ʧ��";
		return false;
	}

	auto root = doc.child("Config");
	// SIP ������
	auto sip_server_node = root.child("SipServer");
	if (sip_server_node) {
		server_info = std::make_shared<SipServerInfo>();
		// SIP ������IP
		server_info->IP = sip_server_node.child_value("IP");
		// SIP �������̶��˿�
		nbase::StringToInt(sip_server_node.child_value("Port"), &server_info->Port);
		// SIP ������ID
		server_info->ID = sip_server_node.child_value("ID");
		// SIP �������̶�����
		server_info->Password = sip_server_node.child_value("Password");
	}
	else {
		LOG(ERROR) << "SipServer�ڵ����";
		return false;
	}

	//��ý�����
	auto media_server_node = root.child("MediaServer");
	if (media_server_node) {
		media_server_info = std::make_shared<MediaServerInfo>();

		//��ý����� IP
		media_server_info->IP = media_server_node.child_value("IP");
		//��ý����� �˿�
		nbase::StringToInt(media_server_node.child_value("Port"), &media_server_info->Port);
		//��ý����� �������127.0.0.1�Ļ�����ҪУ��Secret�ֶ�
		media_server_info->Secret = media_server_node.child_value("Secret");
	}
	else {
		LOG(ERROR) << "MediaServer�ڵ����";
		return false;
	}


	//�豸�б���ǰ�������Ҫ��������
	auto http_server_node = root.child("HttpServer");
	if (http_server_node) {
		nbase::StringToInt(http_server_node.child_value("Port"), &http_port);
	}

	//�豸�б���ǰ�������Ҫ��������
	auto devicelist_node = root.child("DeviceList");
	if (devicelist_node) {
		auto device_nodes = devicelist_node.children("Device");
		for (auto&& device_node : device_nodes) {
			auto device_info = std::make_shared<DeviceInfo>();

			device_info->IP = device_node.child_value("IP");
			nbase::StringToInt(device_node.child_value("Port"), &device_info->Port);
			//��ǰ�豸ID
			device_info->ID = device_node.child_value("ID");
			//�豸����
			device_info->Name = device_node.child_value("Name");
			//����
			device_info->Manufacturer = device_node.child_value("Manufacturer");
			// SIP����Э��
			std::string text = device_node.child_value("Protocol");
			device_info->Protocol = (text.compare("TCP") == 0 ? IPPROTO_TCP : IPPROTO_UDP);
			//�������ʱ��
			nbase::StringToInt(device_node.child_value("HeartbeatInterval"), &device_info->HeartbeatInterval);
			//�豸Ŀ¼��һ�������ֻ��һ��ͨ��������һ���豸�� ���ų�ĳЩ����»��ж�̨�豸�����
			auto catalog_node = device_node.child("Catalog");
			if (catalog_node) {
				auto channel_nodes = catalog_node.children("Channel");
				for (auto&& node : channel_nodes) {
					auto channel_info = std::make_shared<ChannelInfo>();
					channel_info->ID = node.child_value("ID");
					channel_info->Name = node.child_value("Name");
					std::string text = node.child_value("URI");
					nbase::StringTrim(text);
					auto pos = text.find_first_of('/');
					if (pos > 0 && pos != std::string::npos) {
						channel_info->App = text.substr(0, pos);
						channel_info->Stream = text.substr(pos + 1);
					}
					device_info->Channels.push_back(channel_info);
				}
				devices.push_back(device_info);
			}
		}
	}
	else {
		LOG(ERROR) << "Devices�ڵ����";
		return false;
	}
	LOG(INFO) << "�����ļ��������";
	return true;
}