#ifndef STOP_WAIT_RDT_RECEIVER_H
#define STOP_WAIT_RDT_RECEIVER_H
#include <vector>
#include <fstream>
#include "RdtReceiver.h"
//#include "Config.h"

//extern fstream file;
class SRRdtReceiver :public RdtReceiver
{
private:
	int WindowsSize;
	int base;
	int expectSequenceNumberRcvd;	//ȷ�ϱ������
	Packet lastAckPkt;				//�ϴη��͵�ȷ�ϱ���
	const int SeqSize;				//�������

	std::vector <Packet> PacketWindows;
	std::vector <int> AckWindows;
	std::fstream file;
public:
	SRRdtReceiver();
	virtual ~SRRdtReceiver();

public:

	void receive(Packet &packet);	//���ձ��ģ�����NetworkService����
};

#endif

