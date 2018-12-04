#include "stdafx.h"
#include "Global.h"
#include "TCPRdtSender.h"

TCPRdtSender::TCPRdtSender() :WindowsSize(4), SeqSize(8), expectSequenceNumberSend(0), waitingState(false), base(0),AckNumber(0)
{
	//PackWindow = std::vector<Packet>(WindowsSize);
	file.open("TCPwindows.txt", ofstream::out);
}

TCPRdtSender::~TCPRdtSender()
{}

bool TCPRdtSender::send(Message &message)
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

	expectSequenceNumberSend = (expectSequenceNumberSend + 1) % SeqSize;

	if ((int)PacketWindows.size() == WindowsSize)
		waitingState = true;

	return true;
}

//����ȷ��Ack������NetworkServiceSimulator����	
void TCPRdtSender::receive(Packet &ackPkt)
{
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//ackPkt.acknum == this->PacketWindows[0].seqnum
	//���У�����ȷ������ȷ�����=���ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum)
	{
		//this->expectSequenceNumberSend = (this->expectSequenceNumberSend) % SeqSize;			//��һ�����������0-1֮���л�
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);

		//���ack�Ƿ�����ȷ��Χ
		int flag = 0;
		if (base <= expectSequenceNumberSend)
		{
			if (ackPkt.acknum > base &&ackPkt.acknum <= expectSequenceNumberSend)
				flag = 1;
		}
		else
		{
			if (ackPkt.acknum > base)
				flag = 1;
			else if (ackPkt.acknum <= expectSequenceNumberSend)
				flag = 1;
		}

		if (flag == 1)
		{
			waitingState = false;

			if (PacketWindows.size() == (ackPkt.acknum - base + SeqSize) % SeqSize)
				pns->stopTimer(SENDER, this->PacketWindows[0].seqnum);		//�رն�ʱ��
			else
			{
				pns->stopTimer(SENDER, this->PacketWindows[0].seqnum);									//���ȹرն�ʱ��
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->PacketWindows[(ackPkt.acknum - base + SeqSize) % SeqSize ].seqnum);			//�����������ͷ���ʱ��
			}

			for (int i = 0; i < (ackPkt.acknum - base + SeqSize) % SeqSize; ++i)
				PacketWindows.erase(PacketWindows.begin(), PacketWindows.begin() + 1);

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


			AckNumber = 1;
			base = (ackPkt.acknum) % SeqSize;
		}

		//�����ش�
		else if (ackPkt.acknum == base)
		{
			++AckNumber;
			if (AckNumber >= 3 && PacketWindows.size()!=0)
			{
				//TCP��ʱֻ�ش�һ������
				pUtils->printPacket("��������ACK���ط��ϴη��͵ı���", PacketWindows[0]);

				pns->stopTimer(SENDER, PacketWindows[0].seqnum);										//���ȹرն�ʱ��
				pns->startTimer(SENDER, Configuration::TIME_OUT, PacketWindows[0].seqnum);			//�����������ͷ���ʱ��
				pns->sendToNetworkLayer(RECEIVER, PacketWindows[0]);			//���·������ݰ�

				AckNumber = 0;
				streambuf *backup;
				backup = cout.rdbuf();   // back up cin's streambuf  
				cout.rdbuf(file.rdbuf()); // assign file's streambuf to cin  
				cout << "�յ������ظ�ack�����������ش���\n";
				ackPkt.print();
				cout << "�ش���:\n";
				PacketWindows[0].print();
				cout << endl;
				cout.rdbuf(backup);
			}
		}
	}
	else	pUtils->printPacket("\n���ͷ������յ�ȷ��", ackPkt);

}

//Timeout handler������NetworkServiceSimulator����
void TCPRdtSender::timeoutHandler(int seqNum)
{
	//TCP��ʱֻ�ش�һ������
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط���ʱ�ı���", PacketWindows[0]);

	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, PacketWindows[0].seqnum);			//�����������ͷ���ʱ��
	pns->sendToNetworkLayer(RECEIVER, PacketWindows[0]);			//���·������ݰ�

}

bool TCPRdtSender::getWaitingState()
{
	return waitingState;
}



