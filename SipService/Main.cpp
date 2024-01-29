﻿// GB_Service.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "SipServer.h"
#include "ConfigManager.h"
#include "HttpServer.h"
#include "ZlmServer.h"
#include "SSRC_Config.h"
#include "DbManager.h"
#include "DeviceManager.h"
#include "Utils.h"

int main()
{
	auto root = fs::path(GetCurrentModuleDirectory());
	auto log_path = root /"logs" ;
	auto db_path = root / "record.db";
	auto config_file = root / "server.xml";

	fs::create_directories(log_path);
	// nbase::win32::CreateDirectoryRecursively(nbase::win32::MBCSToUnicode(log_path).c_str());

	google::InitGoogleLogging("");
	google::SetStderrLogging(google::GLOG_INFO);
	google::SetLogDestination(google::GLOG_INFO, (log_path / "/").lexically_normal().c_str());
	google::SetLogFilenameExtension(".log");
	google::EnableLogCleaner(3);

	FLAGS_logbufsecs = 1;
	FLAGS_colorlogtostderr = true;

	auto ret = ConfigManager::GetInstance()->LoadConfig(config_file.string());
	if (!ret)
		return 0;

	DbManager::GetInstance()->Init(db_path.string());
	ZlmServer::GetInstance()->Init(ConfigManager::GetInstance()->GetMediaServerInfo());

	DeviceManager::GetInstance()->Init();
	auto config = ConfigManager::GetInstance()->GetSipServerInfo();
	SSRCConfig::GetInstance()->SetPrefix(config->ID.substr(3, 5));

	SipServer sip_tcp_server, sip_udp_server;

	sip_tcp_server.Init(config->ID, config->Port, true);
	sip_udp_server.Init(config->ID, config->Port, false);
	sip_tcp_server.Start();
	sip_udp_server.Start();

	HttpServer::GetInstance()->Start(ConfigManager::GetInstance()->GetHttpPort());
	DeviceManager::GetInstance()->Start();
#ifdef _DEBUG
	getchar();
#else
	static std::promise<void> s_exit;
	signal(SIGINT, [](int signal)
		{
			LOG(INFO) << "Catch Signal: " << signal;
			s_exit.set_value();
});
	s_exit.get_future().wait();
#endif

	sip_tcp_server.Stop();
	sip_udp_server.Stop();
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
