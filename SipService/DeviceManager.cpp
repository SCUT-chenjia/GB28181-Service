﻿#include "pch.h"
#include "DeviceManager.h"
#include "DbManager.h"

void DeviceManager::Init()
{
	std::scoped_lock<std::mutex> lk(_mutex);
	auto&& devices = DbManager::GetInstance()->GetDeviceList();
	for (auto&& dev : devices)
	{
		_devices[dev->GetDeviceID()] = dev;
		auto&& channels = DbManager::GetInstance()->GetChannelList(dev->GetDeviceID());
		for (auto&& ch : channels)
		{
			dev->InsertChannel(dev->GetDeviceID(), ch->GetChannelID(), ch);
		}
	}
}

void DeviceManager::AddDevice(Device::Ptr device)
{
	std::scoped_lock<std::mutex> lk(_mutex);
	_devices[device->GetDeviceID()] = device;
}

Device::Ptr DeviceManager::GetDevice(const std::string& device_id)
{
	std::scoped_lock<std::mutex> lk(_mutex);
	auto iter = _devices.find(device_id);
	if (iter != _devices.end())
	{
		return iter->second;
	}
	return nullptr;
}

Device::Ptr DeviceManager::GetDevice(const std::string& ip, const std::string& port)
{
	std::scoped_lock<std::mutex> lk(_mutex);
	for (auto&& dev : _devices)
	{
		if (dev.second->GetIP() == ip && dev.second->GetPort() == port)
		{
			return dev.second;
		}
	}
	return nullptr;
}

void DeviceManager::RemoveDevice(const std::string& device_id)
{
	std::scoped_lock<std::mutex> lk(_mutex);
	_devices.erase(device_id);
}

std::vector<Device::Ptr> DeviceManager::GetDeviceList()
{
	std::scoped_lock<std::mutex> lk(_mutex);
	std::vector<Device::Ptr> devices;
	for (auto&& dev : _devices)
	{
		devices.push_back(dev.second);
	}
	return devices;
}

int DeviceManager::GetDeviceCount()
{
	std::scoped_lock<std::mutex> lk(_mutex);
	return (int)_devices.size();
}

void DeviceManager::UpdateDeviceStatus(const std::string& device_id, int status)
{
	std::scoped_lock<std::mutex> lk(_mutex);
	auto iter = _devices.find(device_id);
	if (iter != _devices.end())
	{
		iter->second->SetStatus(status);
	}
}

void DeviceManager::UpdateDeviceLastTime(const std::string& device_id, time_t time)
{
	std::scoped_lock<std::mutex> lk(_mutex);
	auto iter = _devices.find(device_id);
	if (iter != _devices.end())
	{
		iter->second->UpdateLastTime(time);
	}
}

void DeviceManager::UpdateDeviceChannelCount(const std::string& device_id, int count)
{
	std::scoped_lock<std::mutex> lk(_mutex);
	auto iter = _devices.find(device_id);
	if (iter != _devices.end())
	{
		iter->second->SetChannelCount(count);
	}
}

void DeviceManager::Start()
{
	CheckDeviceStatus();
}

void DeviceManager::CheckDeviceStatus()
{
	_check_timer.reset(new toolkit::Timer(
		60,
		[this]()
		{
			std::scoped_lock<std::mutex> lk(_mutex);
			time_t now = time(nullptr);

			//TODO： 心跳超时判断
			for (auto&& dev : _devices)
			{
				if (now - dev.second->GetLastTime() > 60 * 3)
				{
					dev.second->SetStatus(0);
				}
			}

			return true;
		},
		nullptr)
	);
}
