#include "pch.h"
#include "ConfigManager.h"
#include "Utils.h"

bool ConfigManager::LoadConfig(std::wstring filepath)
{
	LOG(INFO) << "�����ļ�����...";
	pugi::xml_document doc;
	auto ret = doc.load_file(filepath.c_str(), pugi::parse_full);
	if (ret.status != pugi::status_ok)
	{
		LOG(ERROR) << "�����ļ�����ʧ��";
		return false;
	}

	auto root = doc.child("Config");
	// SIP ������
	auto sip_server_node = root.child("SipServer");
	if (sip_server_node)
	{
		server_info = std::make_shared<SipServerInfo>();
		// SIP ������IP
		server_info->IP = sip_server_node.child_value("IP");
		// SIP �������̶��˿�
		server_info->Port = sip_server_node.child("Port").text().as_int();
		// SIP ������ID
		server_info->ID = sip_server_node.child_value("ID");
		// ȡID��ǰ10λ
		server_info->Realm = server_info->ID.substr(0, 10);
		// ÿ�����е�ʱ���������
		server_info->Nonce = GenerateRandomString(16);
		//LOG(INFO) << server_info->Nonce;
		// SIP �������̶�����
		server_info->Password = sip_server_node.child_value("Password");
		//��ý�����������IP
		server_info->ExternIP = sip_server_node.child_value("ExternIP");
	}
	else
	{
		LOG(ERROR) << "SipServer�ڵ����";
		return false;
	}

	//��ý�����
	auto media_server_node = root.child("MediaServer");
	if (media_server_node)
	{
		media_server_info = std::make_shared<MediaServerInfo>();

		//��ý����� IP
		media_server_info->IP = media_server_node.child_value("IP");
		//��ý����� �˿�
		media_server_info->Port = media_server_node.child("Port").text().as_int();
		//��ý����� �������127.0.0.1�Ļ�����ҪУ��Secret�ֶ�
		media_server_info->Secret = media_server_node.child_value("Secret");

		media_server_info->SinglePortMode = media_server_node.child("SinglePortMode").text().as_bool();

		media_server_info->RtpPort = media_server_node.child("RtpPort").text().as_int();
	}
	else
	{
		LOG(ERROR) << "MediaServer�ڵ����";
		return false;
	}


	//����Http����
	auto http_node = root.child("Http");
	if (http_node)
	{
		//��ý����� �˿�
		http_port = http_node.child("Port").text().as_int();
	}
	else
	{
		LOG(ERROR) << "Http�ڵ����";
		return false;
	}

	LOG(INFO) << "�����ļ��������";
	return true;
}