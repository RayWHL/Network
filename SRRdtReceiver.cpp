#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"

SRRdtReceiver::SRRdtReceiver() :WindowsSize(4),base(0), SeqSize(8)
{
	file.open("SRwindows2.txt", ofstream::out);
	expectSequenceNumberRcvd = WindowsSize;
	PacketWindows = std::vector <Packet>(WindowsSize);
	AckWindows = std::vector<int>(WindowsSize);
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


SRRdtReceiver::~SRRdtReceiver()
{
}

void SRRdtReceiver::receive(Packet &packet) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	//如果校验和正确，同时收到报文的序号等于接收方期待收到的报文序号一致
	if (checkSum == packet.checksum) {
		pUtils->printPacket("接收方正确收到发送方的报文", packet);

		
		//检测ack是否在正确范围
		int flag = 0;
		if (base <= expectSequenceNumberRcvd)
		{
			if (packet.acknum >= base &&packet.acknum < expectSequenceNumberRcvd)
				flag = 1;
		}
		else
		{
			if (packet.acknum >= base)
				flag = 1;
			else if (packet.acknum < expectSequenceNumberRcvd)
				flag = 1;
		}

		if (flag == 1)
		{
			int i;
			for (i = 0; i < (int)PacketWindows.size(); ++i)
			{
				if (PacketWindows[i].seqnum == packet.seqnum)
					break;
			}
			//窗口中无该分组 放入分组
			if (i == PacketWindows.size())
			{
				PacketWindows[(packet.seqnum - base+SeqSize)%SeqSize] = packet;
				AckWindows[(packet.seqnum - base+SeqSize)%SeqSize] = 1;

			}
			else
			{
				//未确认
				if (AckWindows[i] == 0)
				{
					PacketWindows[(packet.seqnum - base + SeqSize) % SeqSize] = packet;
					AckWindows[(packet.seqnum - base + SeqSize) % SeqSize] = 1;
				}

			}
			lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("接收方发送确认报文", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
			
			int changeflag = 0;		//窗口移动标志
			for (int i = 0; i <(int) AckWindows.size(); ++i)
			{
				if (AckWindows[0] == 1)
				{
					changeflag = 1;
					//取出Message，向上递交给应用层
					Message msg;
					memcpy(msg.data, PacketWindows[0].payload, sizeof(PacketWindows[0].payload));
					pns->delivertoAppLayer(RECEIVER, msg);
					AckWindows[0] = 0;

					PacketWindows.push_back(PacketWindows[0]);
					PacketWindows.erase(PacketWindows.begin(), PacketWindows.begin() + 1);
					AckWindows.push_back(0);
					AckWindows.erase(AckWindows.begin(), AckWindows.begin()+1);
					base = (base + 1) % SeqSize;
					expectSequenceNumberRcvd = (expectSequenceNumberRcvd + 1) % SeqSize;
				}
				else
					break;
			}

			
				streambuf *backup;
				backup = cout.rdbuf();   // back up cin's streambuf  
				cout.rdbuf(file.rdbuf()); // assign file's streambuf to cin  
				cout << "接受方收到确认";
				packet.print();
				cout << "\n接受发窗口移动：\n";
				cout << "剩余元素为：\n";
				for (int changei = 0; changei <(int)PacketWindows.size(); ++changei)
				{
					if (AckWindows[changei] == 1)
					{
						cout << "序号：" << PacketWindows[changei].seqnum << " ";
						PacketWindows[changei].print();
					}
				}
				cout << endl;
				cout.rdbuf(backup);

				
			
			
		}
		else
		{
			lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("接收方发送确认报文", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方

		}
	}
	else {


		pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);

		pUtils->printPacket("接收方忽略该报文", lastAckPkt);
		//pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方

	}
}