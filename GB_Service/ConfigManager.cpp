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
        server_info->IP = "0.0.0.0";
        // SIP �������̶��˿�
        nbase::StringToInt(sip_server_node.child_value("Port"), &server_info->Port);
        // SIP ������ID
        server_info->ID = sip_server_node.child_value("ID");
        server_info->Domain = sip_server_node.child_value("Domain");
        server_info->Nonce = GenerateRandomString(16);
        // SIP �������̶�����
        server_info->Password = sip_server_node.child_value("Password");
    } else {
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
    } else {
        LOG(ERROR) << "MediaServer�ڵ����";
        return false;
    }

    LOG(INFO) << "�����ļ��������";
    return true;
}