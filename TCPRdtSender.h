#ifndef STOP_WAIT_RDT_SENDER_H
#define STOP_WAIT_RDT_SENDER_H
#include <vector>
#include <fstream>
#include "RdtSender.h"
class TCPRdtSender :public RdtSender
{
private:
	const int WindowsSize;
	const int SeqSize;				//�������
	int expectSequenceNumberSend;	// ��һ��������� 
	int base;						//�����
	bool waitingState;				// �Ƿ��ڵȴ�Ack��״̬

	int AckNumber;					//�����ش�
	Packet packetWaitingAck;		//�ѷ��Ͳ��ȴ�Ack�����ݰ�

	std::vector<Packet> PacketWindows;
	std::fstream file;
public:

	bool getWaitingState();
	bool send(Message &message);						//����Ӧ�ò�������Message����NetworkServiceSimulator����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	void receive(Packet &ackPkt);						//����ȷ��Ack������NetworkServiceSimulator����	
	void timeoutHandler(int seqNum);					//Timeout handler������NetworkServiceSimulator����

public:
	TCPRdtSender();
	virtual ~TCPRdtSender();
};

#endif

