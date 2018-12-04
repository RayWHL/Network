#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"

SRRdtSender::SRRdtSender() :WindowsSize(4), SeqSize(8), expectSequenceNumberSend(0), waitingState(false), base(0)
{
	//PackWindow = std::vector<Packet>(WindowsSize);
	file.open("SRwindows1.txt",ofstream::out);
}

SRRdtSender::~SRRdtSender()
{}

bool SRRdtSender::send(Message &message)
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
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);			//�������ͷ���ʱ��

	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�

	PacketWindows.push_back(packetWaitingAck);
	AckWindows.push_back(0);

	streambuf *backup;
	backup = cout.rdbuf();   // back up cin's streambuf  
	cout.rdbuf(file.rdbuf()); // assign file's streambuf to cin  
							  // ... cin will read from file  
	cout << "���ͷ����ͷ���:";
	packetWaitingAck.print();
	cout << "��������:\n";
	for (int pi = 0; pi < PacketWindows.size(); ++pi)
	{
		cout << "��ţ�" << PacketWindows[pi].seqnum << " ";
		PacketWindows[pi].print();
	}
	cout << "\n";
	cout.rdbuf(backup);

	//if (base == expectSequenceNumberSend)
	//	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);			//�����������ͷ���ʱ��


	expectSequenceNumberSend = (expectSequenceNumberSend + 1) % SeqSize;

	if ((int)PacketWindows.size() == WindowsSize)
		waitingState = true;

	return true;
}

//����ȷ��Ack������NetworkServiceSimulator����	
void SRRdtSender::receive(Packet &ackPkt)
{
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//ackPkt.acknum == this->PacketWindows[0].seqnum
	//���У�����ȷ������ȷ�����=���ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum)
	{
		//this->expectSequenceNumberSend = (this->expectSequenceNumberSend) % SeqSize;			//��һ�����������0-1֮���л�

		int i;

		for (i = 0; i < (int)PacketWindows.size(); ++i)
		{
			if (PacketWindows[i].seqnum == ackPkt.acknum)
				break;
		}
		//δȷ�ϴ��ں��и����
		if (i != PacketWindows.size())
		{
			pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);

			//�����δȷ��
			if (AckWindows[i] == 0)
			{
				pns->stopTimer(SENDER, ackPkt.acknum);		//�رն�ʱ��
				AckWindows[i] = 1;


				if (i == 0)
				{
					int changeflag = 0;
					while (PacketWindows.size()>0)
					{
						if (AckWindows[0] == 1)
						{
							PacketWindows.erase(PacketWindows.begin(), PacketWindows.begin() + 1);
							AckWindows.erase(AckWindows.begin(), AckWindows.begin() + 1);
							base = (ackPkt.acknum + 1) % SeqSize;
							changeflag = 1;
						}
						else
							break;

					}

					if (changeflag == 1)
					{
						streambuf *backup;
						backup = cout.rdbuf();   // back up cin's streambuf  
						cout.rdbuf(file.rdbuf()); // assign file's streambuf to cin  
						cout << "���ͷ��յ�ȷ��";
						ackPkt.print();
						cout << "\n���ͷ������ƶ���ʣ��" << PacketWindows.size() << "��δȷ��Ԫ��\n";
						cout << "ʣ��Ԫ��Ϊ��\n";
						for (int i = 0; i <(int)PacketWindows.size(); ++i)
						{
							cout << "��ţ�" << PacketWindows[i].seqnum << " ";
							PacketWindows[i].print();
						}
						cout << endl;
						cout.rdbuf(backup);
						
					}
				}
				//�жϴ����Ƿ���
				if (PacketWindows.size() != WindowsSize)
					waitingState = false;

			}
		}
		else
			pUtils->printPacket("���ͷ���ȷ�յ�ȷ�ϰ�������Ŵ���", ackPkt);

	}
	else	pUtils->printPacket("\n���ͷ������յ�ȷ��", ackPkt);

}

//Timeout handler������NetworkServiceSimulator����
void SRRdtSender::timeoutHandler(int seqNum)
{
	int i;
	for (i = 0; i < (int)PacketWindows.size(); ++i)
	{
		if (PacketWindows[i].seqnum == seqNum)
			break;
	}

	if (i != PacketWindows.size())
	{
		pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط���ʱ�ı���", PacketWindows[i]);
		pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
		pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
		pns->sendToNetworkLayer(RECEIVER, PacketWindows[i]);			//���·������ݰ�
	}

}

bool SRRdtSender::getWaitingState()
{
	return waitingState;
}



