#ifndef STOP_WAIT_RDT_RECEIVER_H
#define STOP_WAIT_RDT_RECEIVER_H
#include "RdtReceiver.h"
class GBNRdtReceiver :public RdtReceiver
{
private:
	int expectSequenceNumberRcvd;	//ȷ�ϱ������
	Packet lastAckPkt;				//�ϴη��͵�ȷ�ϱ���
	const int SeqSize;				//�������
public:
	GBNRdtReceiver();
	virtual ~GBNRdtReceiver();

public:

	void receive(Packet &packet);	//���ձ��ģ�����NetworkService����
};

#endif

