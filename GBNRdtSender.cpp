#include "stdafx.h"
#include "Global.h"
#include "GBNRdtSender.h"

GBNRdtSender::GBNRdtSender() :WindowsSize(4), SeqSize(8),expectSequenceNumberSend(0),waitingState(false),base(0)
{
	//PackWindow = std::vector<Packet>(WindowsSize);
	file.open("windows.txt", ofstream::out);
}

GBNRdtSender::~GBNRdtSender()
{}

bool GBNRdtSender::send(Message &message)
{
	if (waitingState)
	{
		Packet ErrorPacket;
		memcpy(ErrorPacket.payload, message.data, sizeof(message.data));

		pUtils->printPacket("\n���ʹ���", ErrorPacket);
		return false;
	}
	
	this->packetWaitingAck.acknum = expectSequenceNumberSend; //���Ը��ֶ�

	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck);
	if (base == expectSequenceNumberSend)
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);			//�����������ͷ���ʱ��

	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�

	PacketWindows.push_back(packetWaitingAck);

	expectSequenceNumberSend = (expectSequenceNumberSend + 1) % SeqSize;

	if ((int)PacketWindows.size() == WindowsSize)
		waitingState = true;
	
	streambuf *backup;
	backup = cout.rdbuf();   // back up cin's streambuf  
	cout.rdbuf(file.rdbuf()); // assign file's streambuf to cin  
							// ... cin will read from file  
	cout<<"���ͷ����ͷ���:";
	packetWaitingAck.print();
	cout<< "��������:\n";
	for (int pi = 0; pi < PacketWindows.size(); ++pi)
	{
		cout << "��ţ�" << PacketWindows[pi].seqnum << " ";
		PacketWindows[pi].print();
	}
	cout << "\n";
	cout.rdbuf(backup);

	return true;
}
 
//����ȷ��Ack������NetworkServiceSimulator����	
void GBNRdtSender::receive(Packet &ackPkt)
{
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//ackPkt.acknum == this->PacketWindows[0].seqnum
	//���У�����ȷ������ȷ�����=���ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum )
	{
		//this->expectSequenceNumberSend = (this->expectSequenceNumberSend) % SeqSize;			//��һ�����������0-1֮���л�
		//���ack�Ƿ�����ȷ��Χ
		int flag = 0;
		if (base <= expectSequenceNumberSend)
		{
			if (ackPkt.acknum >= base &&ackPkt.acknum < expectSequenceNumberSend)
				flag = 1;
		}
		else
		{
			if (ackPkt.acknum >= base)
				flag = 1;
			else if (ackPkt.acknum < expectSequenceNumberSend)
				flag = 1;
		}

		if (flag==1)
		{
			pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);

			waitingState = false;

			if (PacketWindows.size() == (ackPkt.acknum - base + SeqSize) % SeqSize + 1)
				pns->stopTimer(SENDER, this->PacketWindows[0].seqnum);		//�رն�ʱ��
			else
			{
				pns->stopTimer(SENDER, this->PacketWindows[0].seqnum);									//���ȹرն�ʱ��
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->PacketWindows[(ackPkt.acknum - base + SeqSize) % SeqSize + 1].seqnum);			//�����������ͷ���ʱ��
			}

			for (int i = 0; i <= (ackPkt.acknum - base + SeqSize) % SeqSize; ++i)
				PacketWindows.erase(PacketWindows.begin(), PacketWindows.begin() + 1);

			streambuf *backup;
			backup = cout.rdbuf();   // back up cin's streambuf  
			cout.rdbuf(file.rdbuf()); // assign file's streambuf to cin  
			cout << "���ͷ��յ�ȷ��";
			ackPkt.print();
			cout << "\n���ͷ������ƶ���ʣ��" << PacketWindows.size() << "��δȷ��Ԫ��\n";
			cout << "ʣ��Ԫ��Ϊ��\n";
			for (int i = 0; i <(int) PacketWindows.size(); ++i)
			{
				cout << "��ţ�" << PacketWindows[i].seqnum << " ";
				PacketWindows[i].print();
			}
			cout << endl;
			cout.rdbuf(backup);

			/*file << "���ͷ����ܷ���ȷ��:";
			file << ackPkt.acknum;
			file << "��������:\n";
			for (int pi = 0; pi < PacketWindows.size(); ++pi)
			{
				file << "��ţ�" << PacketWindows[pi].seqnum << " " << PacketWindows[pi].payload;
			}
			file << "\n";*/

			base = (ackPkt.acknum + 1) % SeqSize;
		}
		else
		{
			pUtils->printPacket("���ͷ���ȷ�յ�ȷ�ϰ�,������Ŵ���", ackPkt);
		}
	}
	else	pUtils->printPacket("\n���ͷ������յ�ȷ��", ackPkt);
	
}

//Timeout handler������NetworkServiceSimulator����
void GBNRdtSender::timeoutHandler(int seqNum)
{
	printf("��ʱ��ʱ�䵽\n");
	for (int i = 0; i < (int) PacketWindows.size(); ++i)
	{
		pUtils->printPacket("��ʱ�ش� ", PacketWindows[i]);
		
		pns->sendToNetworkLayer(RECEIVER, PacketWindows[i]);			//���·������ݰ�

	}
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
		pns->startTimer(SENDER, Configuration::TIME_OUT, PacketWindows[0].seqnum);			//�����������ͷ���ʱ��
}

bool GBNRdtSender::getWaitingState()
{
	return waitingState;
}



