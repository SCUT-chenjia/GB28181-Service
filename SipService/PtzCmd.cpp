#include "pch.h"
#include "PtzCmd.h"
#include <iomanip>

std::string PtzCmd::cmdString(int leftRight, int upDown, int inOut, int moveSpeed, int zoomSpeed)
{
	int cmdCode = 0;
	if (leftRight == 2) {
		cmdCode = 0x01;  // ����
	}
	else if (leftRight == 1) {
		cmdCode = 0x02;  // ����
	}
	if (upDown == 2) {
		cmdCode |= 0x04;  // ����
	}
	else if (upDown == 1) {
		cmdCode |= 0x08;  // ����
	}
	if (inOut == 2) {
		cmdCode |= 0x10;  // �Ŵ�
	}
	else if (inOut == 1) {
		cmdCode |= 0x20;  // ��С
	}

	std::stringstream ss;
	// ǰ���ֽ�
	ss << "A50F01";
	// �ֽ�4 ָ����
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << cmdCode;
	// �ֽ�5 ˮƽ�����ٶ�
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << moveSpeed;
	// �ֽ�6 ��ֱ�����ٶ�
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << moveSpeed;
	// �ֽ�7 ����λ ��������ٶ�
	ss << std::setfill('0') << std::setw(1) << std::hex << std::uppercase << zoomSpeed;
	// �ֽ�7 ����λ
	ss << "0";
	// �ֽ�8 У����  �ֽ�8=(�ֽ�1+�ֽ�2+�ֽ�3+�ֽ�4+�ֽ�5+�ֽ�6+�ֽ�7)%256
	int checkCode =
		(0xA5 + 0x0F + 0x01 + cmdCode + moveSpeed + moveSpeed + (zoomSpeed & 0xF0)) % 0x100;
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << checkCode;

	return ss.str();
}

std::string PtzCmd::cmdCode(int fourthByte, int fifthByte, int sixthByte, int seventhByte)
{
	std::stringstream ss;
	// ǰ���ֽ�
	ss << "A50F01";
	// �ֽ�4 ָ����
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << fourthByte;
	// �ֽ�5 ˮƽ�����ٶ�
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << fifthByte;
	// �ֽ�6 ��ֱ�����ٶ�
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << sixthByte;
	// �ֽ�7 ����λ ��������ٶ�
	ss << std::setfill('0') << std::setw(1) << std::hex << std::uppercase << seventhByte;
	// �ֽ�7 ����λ
	ss << "0";
	// �ֽ�8 У����  �ֽ�8=(�ֽ�1+�ֽ�2+�ֽ�3+�ֽ�4+�ֽ�5+�ֽ�6+�ֽ�7)%256
	int checkCode =
		(0xA5 + 0x0F + 0x01 + fourthByte + fifthByte + sixthByte + (seventhByte & 0xF0)) % 0x100;
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << checkCode;

	return ss.str();
}
