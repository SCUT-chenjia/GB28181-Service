﻿#include "pch.h"
#include "ConfigManager.h"

bool ConfigManager::LoadConfig(std::string filepath)
{
	SPDLOG_INFO("配置文件解析...");
	pugi::xml_document doc;
	auto ret = doc.load_file(filepath.c_str(), pugi::parse_full);
	if (ret.status != pugi::status_ok)
	{
		SPDLOG_ERROR("配置文件解析失败:{}@{}", ret.description(), ret.offset);
		return false;
	}

	auto root = doc.child("Config");
	// SIP 服务器
	auto sip_server_node = root.child("SipServer");
	if (sip_server_node)
	{
		_server_info = std::make_shared<SipServerInfo>();
		// SIP 服务器IP
		_server_info->IP = sip_server_node.child_value("IP");
		// SIP 服务器固定端口
		_server_info->Port = sip_server_node.child("Port").text().as_int();
		// SIP 服务器ID
		_server_info->ID = sip_server_node.child_value("ID");
		// SIP 服务器固定密码
		_server_info->Password = sip_server_node.child_value("Password");
	}
	else
	{
		SPDLOG_ERROR("SipServer节点错误");
		return false;
	}

	//流媒体服务
	auto media_server_node = root.child("MediaServer");
	if (media_server_node)
	{
		_media_server_info = std::make_shared<MediaServerInfo>();

		//流媒体服务 IP
		_media_server_info->IP = media_server_node.child_value("IP");
		//流媒体服务 端口
		_media_server_info->Port = media_server_node.child("Port").text().as_int();
		//流媒体服务 如果不是127.0.0.1的话，需要校验Secret字段
		_media_server_info->Secret = media_server_node.child_value("Secret");
	}
	else
	{
		SPDLOG_ERROR("MediaServer节点错误");
		return false;
	}


	//设备列表，当前软件的主要工作内容
	auto http_server_node = root.child("HttpServer");
	if (http_server_node)
	{
		nbase::StringToInt(http_server_node.child_value("Port"), &http_port);
	}

	//设备列表，当前软件的主要工作内容
	auto devicelist_node = root.child("DeviceList");
	if (devicelist_node)
	{
		auto device_nodes = devicelist_node.children("Device");
		for (auto&& device_node : device_nodes)
		{
			auto device_info = std::make_shared<DeviceInfo>();

			device_info->IP = device_node.child_value("IP");
			device_info->Port = device_node.child("Port").text().as_int(0);
			//当前设备ID
			device_info->ID = device_node.child_value("ID");
			//设备名称
			device_info->Name = device_node.child_value("Name");
			//厂家
			device_info->Manufacturer = device_node.child_value("Manufacturer");
			// SIP传输协议
			std::string text = device_node.child_value("Protocol");
			device_info->Protocol = (text.compare("TCP") == 0 ? IPPROTO_TCP : IPPROTO_UDP);
			//心跳间隔时间
			nbase::StringToInt(device_node.child_value("HeartbeatInterval"), &device_info->HeartbeatInterval);
			//当收到一个bye时，结束所有推流
			device_info->CloseAllWhenBye = device_node.child("CloseAllWhenBye").text().as_bool();

			//设备目录，一般情况下只有一个通道，代表一个设备， 不排除某些情况下会有多台设备的情况
			auto catalog_node = device_node.child("Catalog");
			if (catalog_node)
			{
				auto channel_nodes = catalog_node.children("Channel");
				for (auto&& node : channel_nodes)
				{
					auto channel_info = std::make_shared<ChannelInfo>();
					channel_info->ID = node.child_value("ID");
					channel_info->Name = node.child_value("Name");
					std::string text = node.child_value("URI");
					nbase::StringTrim(text);
					auto pos = text.find_first_of('/');
					if (pos > 0 && pos != std::string::npos)
					{
						channel_info->App = text.substr(0, pos);
						channel_info->Stream = text.substr(pos + 1);
					}
					device_info->Channels.push_back(channel_info);
				}
				_vec_devices.push_back(device_info);
			}
		}
	}
	else
	{
		SPDLOG_ERROR("Devices节点错误");
		return false;
	}

	auto distribute_node = root.child("Distribute");
	if (distribute_node)
	{
		auto nodes = distribute_node.children("Item");
		for (auto&& node : nodes)
		{
			auto item = std::make_shared<DistributeItem>();
			item->Source = node.child_value("Source");

			std::string text = node.child_value("URI");
			nbase::StringTrim(text);
			auto pos = text.find_first_of('/');
			if (pos > 0 && pos != std::string::npos)
			{
				item->App = text.substr(0, pos);
				item->Stream = text.substr(pos + 1);
			}
			else
			{
				continue;
			}

			text = node.child_value("Protocol");
			item->Protocol = (text.compare("UDP") == 0 ? 1 : 0);
			item->RecordMP4 = node.child("RecordMP4").text().as_bool();
			item->RetryTimes = node.child("RetryTimes").text().as_int(-1);

			_vec_distribute_items.push_back(item);
		}
	}

	SPDLOG_INFO("配置文件解析完成");
	return true;
}
