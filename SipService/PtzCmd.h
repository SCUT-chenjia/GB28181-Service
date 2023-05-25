#pragma once
class PtzCmd
{
public:
	typedef std::shared_ptr<PtzCmd> Ptr;


    /**
     * ��ָ̨�������
     *
     * @param leftRight  ��ͷ�������� 0:ֹͣ 1:���� 2:����
     * @param upDown     ��ͷ�������� 0:ֹͣ 1:���� 2:����
     * @param inOut      ��ͷ�Ŵ���С 0:ֹͣ 1:��С 2:�Ŵ�
     * @param moveSpeed  ��ͷ�ƶ��ٶ� Ĭ�� 0XFF (0-255)
     * @param zoomSpeed  ��ͷ�����ٶ� Ĭ�� 0X1 (0-255)
     */
    static std::string cmdString(int leftRight, int upDown, int inOut, int moveSpeed, int zoomSpeed);

    /**
     * @brief ��ָ̨�������
     *
     * @param fourthByte ���ĸ��ֽ�
     * @param fifthByte  ������ֽ�
     * @param sixthByte  �������ֽ�
     * @param seventhByte  ���߸��ֽ�
     * @return std::string
     */
    static std::string cmdCode(int fourthByte, int fifthByte, int sixthByte, int seventhByte);


};

