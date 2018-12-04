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

		pUtils->printPacket("\n发送错误", ErrorPacket);
		return false;
	}
	this->packetWaitingAck.acknum = expectSequenceNumberSend; //忽略该字段

	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	pUtils->printPacket("发送方发送报文", this->packetWaitingAck);
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);			//启动发送方定时器

	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方

	PacketWindows.push_back(packetWaitingAck);
	AckWindows.push_back(0);

	streambuf *backup;
	backup = cout.rdbuf();   // back up cin's streambuf  
	cout.rdbuf(file.rdbuf()); // assign file's streambuf to cin  
							  // ... cin will read from file  
	cout << "发送方发送分组:";
	packetWaitingAck.print();
	cout << "窗口内容:\n";
	for (int pi = 0; pi < PacketWindows.size(); ++pi)
	{
		cout << "序号：" << PacketWindows[pi].seqnum << " ";
		PacketWindows[pi].print();
	}
	cout << "\n";
	cout.rdbuf(backup);

	//if (base == expectSequenceNumberSend)
	//	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);			//重新启动发送方定时器


	expectSequenceNumberSend = (expectSequenceNumberSend + 1) % SeqSize;

	if ((int)PacketWindows.size() == WindowsSize)
		waitingState = true;

	return true;
}

//接受确认Ack，将被NetworkServiceSimulator调用	
void SRRdtSender::receive(Packet &ackPkt)
{
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//ackPkt.acknum == this->PacketWindows[0].seqnum
	//如果校验和正确，并且确认序号=发送方已发送并等待确认的数据包序号
	if (checkSum == ackPkt.checksum)
	{
		//this->expectSequenceNumberSend = (this->expectSequenceNumberSend) % SeqSize;			//下一个发送序号在0-1之间切换

		int i;

		for (i = 0; i < (int)PacketWindows.size(); ++i)
		{
			if (PacketWindows[i].seqnum == ackPkt.acknum)
				break;
		}
		//未确认窗口含有该序号
		if (i != PacketWindows.size())
		{
			pUtils->printPacket("发送方正确收到确认", ackPkt);

			//该序号未确认
			if (AckWindows[i] == 0)
			{
				pns->stopTimer(SENDER, ackPkt.acknum);		//关闭定时器
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
						cout << "发送方收到确认";
						ackPkt.print();
						cout << "\n发送发窗口移动：剩余" << PacketWindows.size() << "个未确认元素\n";
						cout << "剩余元素为：\n";
						for (int i = 0; i <(int)PacketWindows.size(); ++i)
						{
							cout << "序号：" << PacketWindows[i].seqnum << " ";
							PacketWindows[i].print();
						}
						cout << endl;
						cout.rdbuf(backup);
						
					}
				}
				//判断窗口是否满
				if (PacketWindows.size() != WindowsSize)
					waitingState = false;

			}
		}
		else
			pUtils->printPacket("发送方正确收到确认包，但序号错误", ackPkt);

	}
	else	pUtils->printPacket("\n发送方错误收到确认", ackPkt);

}

//Timeout handler，将被NetworkServiceSimulator调用
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
		pUtils->printPacket("发送方定时器时间到，重发超时的报文", PacketWindows[i]);
		pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
		pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
		pns->sendToNetworkLayer(RECEIVER, PacketWindows[i]);			//重新发送数据包
	}

}

bool SRRdtSender::getWaitingState()
{
	return waitingState;
}



