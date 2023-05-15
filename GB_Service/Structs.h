#pragma once
#include "pch.h"

struct SipServerInfo {
    std::string IP;
    int Port;
    std::string ID;
    std::string Domain;
    std::string Password;
    std::string Nonce;
};

struct MediaServerInfo {
    std::string IP;
    int Port;
    std::string Secret;
};
//
//struct ChannelInfo {
//    std::string ID;
//    std::string Name;
//    std::string App;
//    std::string Stream;
//};
//
//struct DeviceInfo {
//    std::string IP;
//    int Port;
//    std::string ID;
//    IPPROTO Protocol;
//    std::string Name;
//    std::string Manufacturer;
//    int HeartbeatInterval;
//
//    std::vector<std::shared_ptr<ChannelInfo>> Channels;
//};
//
//struct SessinInfo {
//    std::string ID;
//    std::string SSRC;
//    std::string TargetIP;
//    int TargetPort;
//    int LocalPort;
//    bool UseTcp = false;
//    std::shared_ptr<ChannelInfo> Channel = nullptr;
//
//    std::string to_string() {
//        return fmt::format(
//            "ID:{}\nSSRC:{}\nDstIP:{}\nDstPort:{}\nLocalPort:{}", ID, SSRC, TargetIP, TargetPort, LocalPort);
//    }
//};



enum REQUEST_MESSAGE_TYPE
{
    REQUEST_TYPE_UNKNOWN = 0,
    KEEPALIVE,                   // ��������
    QUIRY_CATALOG,               //   ��ѯĿ¼
    DEVICE_CONTROL_PTZ,         // �豸����-��̨
    DEVICE_QUIER_PRESET,        // �豸��ѯ-Ԥ��λ
    DEVICE_CONTROL_PRESET,       // �豸����-Ԥ��λ
    DEVICE_CONTROL_HOMEPOSITION, // �豸����-����λ

    REQUEST_CALL_INVITE,            // �㲥
    REQUEST_CALL_PLAYBACK,          // �ط�
    REQUEST_CALL_LIVE,              // ֱ��
    REQUEST_CALL_DOWNLOAD,          // ����
    REQUEST_CALL_BYE,               // �Ҷ�

    REQUEST_TYPE_MAX
};


enum REQ_CALL_TYPE
{
    REQ_CALL_TYPE_UNKNOWN = 0,

    REQ_CALL_TYPE_MAX
};
