#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"

SRRdtReceiver::SRRdtReceiver() :WindowsSize(4),base(0), SeqSize(8)
{
	file.open("SRwindows2.txt", ofstream::out);
	expectSequenceNumberRcvd = WindowsSize;
	PacketWindows = std::vector <Packet>(WindowsSize);
	AckWindows = std::vector<int>(WindowsSize);
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


SRRdtReceiver::~SRRdtReceiver()
{
}

void SRRdtReceiver::receive(Packet &packet) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);

	//���У�����ȷ��ͬʱ�յ����ĵ���ŵ��ڽ��շ��ڴ��յ��ı������һ��
	if (checkSum == packet.checksum) {
		pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);

		
		//���ack�Ƿ�����ȷ��Χ
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
			//�������޸÷��� �������
			if (i == PacketWindows.size())
			{
				PacketWindows[(packet.seqnum - base+SeqSize)%SeqSize] = packet;
				AckWindows[(packet.seqnum - base+SeqSize)%SeqSize] = 1;

			}
			else
			{
				//δȷ��
				if (AckWindows[i] == 0)
				{
					PacketWindows[(packet.seqnum - base + SeqSize) % SeqSize] = packet;
					AckWindows[(packet.seqnum - base + SeqSize) % SeqSize] = 1;
				}

			}
			lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
			
			int changeflag = 0;		//�����ƶ���־
			for (int i = 0; i <(int) AckWindows.size(); ++i)
			{
				if (AckWindows[0] == 1)
				{
					changeflag = 1;
					//ȡ��Message�����ϵݽ���Ӧ�ò�
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
				cout << "���ܷ��յ�ȷ��";
				packet.print();
				cout << "\n���ܷ������ƶ���\n";
				cout << "ʣ��Ԫ��Ϊ��\n";
				for (int changei = 0; changei <(int)PacketWindows.size(); ++changei)
				{
					if (AckWindows[changei] == 1)
					{
						cout << "��ţ�" << PacketWindows[changei].seqnum << " ";
						PacketWindows[changei].print();
					}
				}
				cout << endl;
				cout.rdbuf(backup);

				
			
			
		}
		else
		{
			lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�

		}
	}
	else {


		pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,����У�����", packet);

		pUtils->printPacket("���շ����Ըñ���", lastAckPkt);
		//pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�

	}
}