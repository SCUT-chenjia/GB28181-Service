﻿/*****************************************************************//**
 * \file   HttpClient.h
 * \brief  和流媒体服务器之间的通信
 *
 * \author yszs
 * \date   March 2024
 *********************************************************************/
#pragma once
#include "Structs.h"

namespace media
{
	struct mediaserver_stream_item
	{
		int64_t aliveSecond;    //存活时间
		std::string app;        //媒体流路径
		int64_t bytesSpeed;     //码率，字节每秒
		int64_t createStamp;    //创建的时间戳
		bool isRecordingHLS;    //是否正在录制hls
		bool isRecordingMP4;    //是否正在录制mp4
		std::string originUrl;  //源地址
		int readerCount;        //观看者数量
		int originType;         //源类型
		std::string originTypeStr;  //源类型
		std::string schema;         //媒体类型
		std::string stream;         //媒体流路径
		int codec_id;               //编码器id
		std::string codec_id_name;  //编码器名称
		int fps;                //帧率
		int width;              //宽
		int height;             //高
		std::string vhost;      //虚拟主机名

		//------------------------------
		std::string resolution; //分辨率
		std::string path;       //路径
		std::string url;        //拉流地址
	};

	void to_json(nlohmann::json& j, const mediaserver_stream_item& p);
	void from_json(const nlohmann::json& j, mediaserver_stream_item& p);
}

class HttpClient {
public:
	SINGLETON_DEFINE(HttpClient);

	/// @brief 初始化，传入流媒体服务器信息
	/// @param info 流媒体服务器信息
	void Init(std::shared_ptr<MediaServerInfo> server_info);

	/// @brief 开始发送RTP数据
	/// @param app 媒体流路径
	/// @param stream 媒体流路径
	/// @param ssrc 媒体标识
	/// @param dst_ip 目的地址
	/// @param dst_port 目的端口
	/// @param local_port 本地端口
	/// @param use_tcp 使用TCP/UDP
	/// @return 返回是否成功，如果失败，则服务端无法收到数据，超时之后会自动发送BYE
	bool StartSendRtp(const std::string& app, const std::string& stream, const std::string& ssrc,
	const std::string& dst_ip, int dst_port, int local_port, bool use_tcp = false);

	/// @brief 停止RTP数据发送
	/// @param app 媒体流一级目录
	/// @param stream  媒体流二级目录
	/// @return 是否成功，只有在流媒体服务器不在线时，会返回失败，不影响
	bool StopSendRtp(const std::string& app, const std::string& stream);

	/// @brief 录像回放开始发送RTP数据包
	/// @param channel_info 通道信息
	/// @param ssrc 媒体标识
	/// @param dst_ip 目的地址
	/// @param dst_port 目的端口
	/// @param local_port 本地端口
	/// @param start_time 录像开始时间 unix时间戳
	/// @param end_time 录像结束时间 unix时间戳
	/// @param use_tcp  使用TCP/UDP
	/// @return 返回是否成功，如果失败，则服务端无法收到数据，超时之后会自动发送BYE
	bool StartSendPlaybackRtp(std::shared_ptr<ChannelInfo> channel_info, const std::string& ssrc,
		 const std::string& dst_ip, int dst_port, int local_port,const std::string& start_time,
		 const std::string& end_time, bool use_tcp = false);

	/// @brief 加载MP4文件
	/// @param app 媒体流路径
	/// @param stream 媒体流路径
	/// @param fileapth 文件路径
	/// @return 是否加载成功
	bool LoadMP4File(const std::string& app, const std::string& stream, const std::string& fileapth);

	/// @brief 获取录像文件信息
	/// @param stream 媒体流路径
	/// @param start_time 开始时间 FILETIME
	/// @param end_time 结束时间 FILETIME
	/// @param response 返回的json信息
	/// @return 返回是否调用成功，如果失败，则response为空
	bool GetMp4RecordInfo(const std::string& stream,
		const std::string& start_time, const std::string& end_time, std::string& response);

	/// @brief 设置录像流播放速度
	/// @param app 
	/// @param stream 
	/// @param speed 
	/// @return 返回是否调用成功
	bool SetRecordSpeed(const std::string& app, const std::string& stream, float speed);

	/// @brief 暂停播放，只针对于实时流
	/// @param app 媒体流路径
	/// @param stream 媒体流路径
	/// @param pause 是否暂停
	/// @return 调用是否成功，直接返回true，不影响
	bool SetPause(const std::string& app, const std::string& stream, bool pause);

	/// @brief 设置播放速度，只针对于实时流
	/// @param app 媒体流路径
	/// @param stream 媒体流路径
	/// @param speed 播放速度
	/// @return 调用是否成功，直接返回true，不影响
	bool SetSpeed(const std::string& app, const std::string& stream, float speed);

	/// @brief 获取流列表
	/// @return 所有流信息
	std::vector<media::mediaserver_stream_item> GetMediaList();


	/// @brief 添加拉流代理
	/// @param item 拉流代理参数
	/// @return 是否添加成功
	bool AddDistributeStream(std::shared_ptr<DistributeItem> item);


	/// @brief 开始录制
	/// @param app 媒体流路径
	/// @param stream 媒体流路径
	/// @param is_mp4 录制MP4文件？ 否则为hls
	/// @return 操作是否成功
	bool StartRecord(const std::string& app, const std::string& stream, bool is_mp4 = true);

	/// @brief 停止录制
	/// @param app 媒体流路径
	/// @param stream 媒体流路径
	/// @param is_mp4 录制MP4文件？ 否则为hls
	/// @return 操作是否成功
	bool StopRecord(const std::string& app, const std::string& stream, bool is_mp4 = true);

private:
	//流媒体服务器信息
	std::shared_ptr<MediaServerInfo> _server_info = nullptr;
	//流媒体服务器http地址
	std::string _base_url = "http://127.0.0.1:8000";

private:
	HttpClient() = default;
};