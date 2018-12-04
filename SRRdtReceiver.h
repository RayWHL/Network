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
	int expectSequenceNumberRcvd;	//确认报文序号
	Packet lastAckPkt;				//上次发送的确认报文
	const int SeqSize;				//分组序号

	std::vector <Packet> PacketWindows;
	std::vector <int> AckWindows;
	std::fstream file;
public:
	SRRdtReceiver();
	virtual ~SRRdtReceiver();

public:

	void receive(Packet &packet);	//接收报文，将被NetworkService调用
};

#endif

