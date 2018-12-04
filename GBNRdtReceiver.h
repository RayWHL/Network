#ifndef STOP_WAIT_RDT_RECEIVER_H
#define STOP_WAIT_RDT_RECEIVER_H
#include "RdtReceiver.h"
class GBNRdtReceiver :public RdtReceiver
{
private:
	int expectSequenceNumberRcvd;	//确认报文序号
	Packet lastAckPkt;				//上次发送的确认报文
	const int SeqSize;				//分组序号
public:
	GBNRdtReceiver();
	virtual ~GBNRdtReceiver();

public:

	void receive(Packet &packet);	//接收报文，将被NetworkService调用
};

#endif

