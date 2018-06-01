#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <vector>
using namespace std;

/* ******************************************************************
 
 SELECTIVE REPEAT EMULATOR: VERSION 1.1  J.F.Kurose

**********************************************************************/

struct time_Pkt{
	int seqnum;
	float timeout;
};

struct ack_Pkt{
	int seqnum;
	int ack;
};

static int N;
static int nextSeqNum;		// for A
static int sendBase; 		// Minimum number of packets sent (beginning of sliding window)
static int receiveBase;
float timeout = 25.0;

static std::vector <msg>  messageList;
static std::vector <pkt> rpacketList;
static std::vector <pkt> spacketList;
static std::vector <ack_Pkt> aPackList;
static std::vector <time_Pkt> tPackList;

int getchecksum(struct pkt *packet);
void UpdateAckList(int pkNum);
void UpdateTimeoutList(int pkNum);
int FindListIndex(int pkNum);
int FindTimeoutListIndex(int pkNum);
int FindSenderListIndex(int pkNum);
void UploadPackets();
void UpdateSendBase();
int IsAcknowledged(int pkNum);

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    if (nextSeqNum<sendBase+N)
    {
        messageList.push_back(message);

        struct msg msgToSend=(struct msg)messageList.front();
        struct pkt packet_a;
		struct time_Pkt tPack;
		struct ack_Pkt aPack;
        packet_a.seqnum = nextSeqNum;
        packet_a.acknum = -1;
        strncpy(packet_a.payload, msgToSend.data, sizeof(message));
        int cs = getchecksum(&packet_a);
        packet_a.checksum = cs;
        messageList.erase(messageList.begin());
        spacketList.push_back(packet_a);

        aPack.seqnum=packet_a.seqnum;
        aPack.ack=0;
        aPackList.push_back(aPack);

        tPack.seqnum=packet_a.seqnum;
        tPack.timeout=get_sim_time()+timeout;
        tPackList.push_back(tPack);

        // send packet & start timer
        tolayer3(0, packet_a);
        
        if(sendBase==nextSeqNum){
            starttimer(0, timeout);
    	}
        nextSeqNum++;
    }
    else
    {
        messageList.push_back(message);
        return;
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	
    int cs = getchecksum(&packet);
    
    if (packet.checksum == cs)
    {
    	UpdateAckList(packet.acknum);
    	UpdateTimeoutList(packet.acknum);
        
        if(sendBase==packet.acknum){
        	UpdateSendBase();
        }
    }
    else
        return;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	time_Pkt ap=tPackList.front(); 
	int ind= FindSenderListIndex(ap.seqnum);
	tolayer3(0,spacketList[ind]);
	ap.timeout=get_sim_time()+timeout;
	tPackList.push_back(ap);
	tPackList.erase(tPackList.begin());

	time_Pkt newTimePkt= tPackList.front();
	float nTimeout= newTimePkt.timeout- get_sim_time();
	starttimer(0, nTimeout);
}

void A_init()
{
	sendBase = 1;
	nextSeqNum = 1;
	N= getwinsize();
}

void B_input(struct pkt packet)
{ 	
    int cs = getchecksum(&packet);

    if(packet.checksum==cs){
    	struct pkt  ack;
    	if(packet.seqnum>=receiveBase && packet.seqnum <= receiveBase+N-1)
        {
        	ack.acknum=packet.seqnum;
        	int acs= getchecksum(&ack);
            ack.checksum=acs;
            tolayer3(1, ack);
       
            if(FindListIndex(packet.seqnum)==-1)
            { 
                //if message is not already buffered
                rpacketList.push_back(packet);
        	}
            if(receiveBase==packet.seqnum)
            {
            	UploadPackets();
            }
    	}
    	else if(packet.seqnum>=receiveBase-N && packet.seqnum <= receiveBase-1)
        {
    		ack.acknum=packet.seqnum;
        	int acs= getchecksum(&ack);
            ack.checksum=acs;
            tolayer3(1, ack);
    	}
    	else 
    		return;

    }

}

void B_init()
{
    receiveBase=1;
}

int getchecksum(struct pkt *packet)
{
    char msg[20];
    strncpy(msg, packet->payload, sizeof(msg));
    int checksum = 0;
    if(packet)
    {
        for (int i=0; i<20; i++)
            checksum += (unsigned char)packet->payload[i];
        checksum += packet->seqnum;
        checksum += packet->acknum;
        return checksum;
    }
    else return 0;
}

/* remove the acknowledged packet from the sender list*/
void UpdateAckList(int pkNum)
{
	for (int i=0; i<aPackList.size(); i++)
    {
		if(aPackList[i].seqnum==pkNum)
        {
			aPackList[i].ack=1;
			return;
		}
	}
}

void UpdateTimeoutList(int pkNum)
{
	int ind=FindTimeoutListIndex(pkNum);	
	if(ind!=-1)
    {
		if(ind==0)
        {
            stoptimer(0);
            tPackList.erase(tPackList.begin());
            time_Pkt newTimerPkt=tPackList.front();
            int newTimeout=newTimerPkt.timeout-get_sim_time();
            starttimer(0, newTimeout);
		}
		else
        {
            tPackList.erase(tPackList.begin()+ind);	
		}

	}
	else
		return;
}

/* find the index of the packet in the receiver packet buffer */
int FindListIndex(int pkNum)
{
	int ind=-1;
	for (int i=0; i< rpacketList.size(); i++)
    {
		if (pkNum==rpacketList[i].seqnum)
			return i;
	}
	return ind;
}

/*find the index of the packet in the timeout list */
int FindTimeoutListIndex(int pkNum)
{
	int ind=-1;
	for(int i=0; i< tPackList.size();i++)
    {
		if(pkNum==tPackList[i].seqnum)
			return i;
	}
	return ind;
}

/* Upload the ready packets to layer 5 on receiver side */
void UploadPackets()
{
	while(!rpacketList.empty()){
		int ind= FindListIndex(receiveBase);

		if(ind != -1)
        {
            struct msg message;
            strncpy(message.data, rpacketList[ind].payload, sizeof(message));
            char *messageToSend = message.data;
            tolayer5(1, messageToSend);
            rpacketList.erase(rpacketList.begin()+ind);
            receiveBase++;    
    	}
    	else 
    		return;
	}
}

void UpdateSendBase()
{
	while(!spacketList.empty())
    {
    	int ind= FindSenderListIndex(sendBase);
    	int isAcked=IsAcknowledged(spacketList[ind].seqnum);

    	if(ind!=-1 && isAcked==1)
        {
    		spacketList.erase(spacketList.begin()+ind);
    		sendBase++;
    	
    	}
    	else
    		return;
    }
}

int FindSenderListIndex(int pkNum)
{
    int ind=-1;
	for (int i=0; i< spacketList.size(); i++)
    {
		if (pkNum==spacketList[i].seqnum)
			return i;
	}
	return ind;
}


int IsAcknowledged(int pkNum)
{
	for (int i=0; i< aPackList.size(); i++)
    {
		if(aPackList[i].seqnum==pkNum)
			return aPackList[i].ack;
	}
	return 0;
}