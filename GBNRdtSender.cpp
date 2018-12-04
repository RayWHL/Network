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

		pUtils->printPacket("\n发送错误", ErrorPacket);
		return false;
	}
	
	this->packetWaitingAck.acknum = expectSequenceNumberSend; //忽略该字段

	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	pUtils->printPacket("发送方发送报文", this->packetWaitingAck);
	if (base == expectSequenceNumberSend)
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);			//重新启动发送方定时器

	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方

	PacketWindows.push_back(packetWaitingAck);

	expectSequenceNumberSend = (expectSequenceNumberSend + 1) % SeqSize;

	if ((int)PacketWindows.size() == WindowsSize)
		waitingState = true;
	
	streambuf *backup;
	backup = cout.rdbuf();   // back up cin's streambuf  
	cout.rdbuf(file.rdbuf()); // assign file's streambuf to cin  
							// ... cin will read from file  
	cout<<"发送方发送分组:";
	packetWaitingAck.print();
	cout<< "窗口内容:\n";
	for (int pi = 0; pi < PacketWindows.size(); ++pi)
	{
		cout << "序号：" << PacketWindows[pi].seqnum << " ";
		PacketWindows[pi].print();
	}
	cout << "\n";
	cout.rdbuf(backup);

	return true;
}
 
//接受确认Ack，将被NetworkServiceSimulator调用	
void GBNRdtSender::receive(Packet &ackPkt)
{
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//ackPkt.acknum == this->PacketWindows[0].seqnum
	//如果校验和正确，并且确认序号=发送方已发送并等待确认的数据包序号
	if (checkSum == ackPkt.checksum )
	{
		//this->expectSequenceNumberSend = (this->expectSequenceNumberSend) % SeqSize;			//下一个发送序号在0-1之间切换
		//检测ack是否在正确范围
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
			pUtils->printPacket("发送方正确收到确认", ackPkt);

			waitingState = false;

			if (PacketWindows.size() == (ackPkt.acknum - base + SeqSize) % SeqSize + 1)
				pns->stopTimer(SENDER, this->PacketWindows[0].seqnum);		//关闭定时器
			else
			{
				pns->stopTimer(SENDER, this->PacketWindows[0].seqnum);									//首先关闭定时器
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->PacketWindows[(ackPkt.acknum - base + SeqSize) % SeqSize + 1].seqnum);			//重新启动发送方定时器
			}

			for (int i = 0; i <= (ackPkt.acknum - base + SeqSize) % SeqSize; ++i)
				PacketWindows.erase(PacketWindows.begin(), PacketWindows.begin() + 1);

			streambuf *backup;
			backup = cout.rdbuf();   // back up cin's streambuf  
			cout.rdbuf(file.rdbuf()); // assign file's streambuf to cin  
			cout << "发送方收到确认";
			ackPkt.print();
			cout << "\n发送发窗口移动：剩余" << PacketWindows.size() << "个未确认元素\n";
			cout << "剩余元素为：\n";
			for (int i = 0; i <(int) PacketWindows.size(); ++i)
			{
				cout << "序号：" << PacketWindows[i].seqnum << " ";
				PacketWindows[i].print();
			}
			cout << endl;
			cout.rdbuf(backup);

			/*file << "发送方接受分组确认:";
			file << ackPkt.acknum;
			file << "窗口内容:\n";
			for (int pi = 0; pi < PacketWindows.size(); ++pi)
			{
				file << "序号：" << PacketWindows[pi].seqnum << " " << PacketWindows[pi].payload;
			}
			file << "\n";*/

			base = (ackPkt.acknum + 1) % SeqSize;
		}
		else
		{
			pUtils->printPacket("发送方正确收到确认包,但是序号错误", ackPkt);
		}
	}
	else	pUtils->printPacket("\n发送方错误收到确认", ackPkt);
	
}

//Timeout handler，将被NetworkServiceSimulator调用
void GBNRdtSender::timeoutHandler(int seqNum)
{
	printf("定时器时间到\n");
	for (int i = 0; i < (int) PacketWindows.size(); ++i)
	{
		pUtils->printPacket("超时重传 ", PacketWindows[i]);
		
		pns->sendToNetworkLayer(RECEIVER, PacketWindows[i]);			//重新发送数据包

	}
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
		pns->startTimer(SENDER, Configuration::TIME_OUT, PacketWindows[0].seqnum);			//重新启动发送方定时器
}

bool GBNRdtSender::getWaitingState()
{
	return waitingState;
}



