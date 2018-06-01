 #include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <vector>
using namespace std;

/* ******************************************************************

GO-BACK-N NETWORK EMULATOR: VERSION 1.1 

**********************************************************************/

static int N;

struct pkt *packet;
static struct pkt  ack;
static struct pkt packet_a;
static int nextSeqNum;			// for A
static int expectedSeqnumB;		// for B
static std::vector <msg> messageList;
static std::vector <pkt> packetList;

int base; 				// Minimum number of packets sent (beginning of sliding window)
float timeout = 25.0;

int getchecksum(struct pkt *packet);

// called from layer 5
void A_output(struct msg message)
{
    if (nextSeqNum<base+N)
    {
        messageList.push_back(message);
        
        // build the packet
        struct msg msgToSend = (struct msg)messageList.front();
        packet_a.seqnum = nextSeqNum;
        packet_a.acknum = -1;
        strncpy(packet_a.payload, msgToSend.data, sizeof(message));
        int cs = getchecksum(&packet_a);
        packet_a.checksum = cs;
        messageList.erase(messageList.begin());
        
        // send packet & start timer
        tolayer3(0, packet_a);
        packetList.push_back(packet_a);
        if(base==nextSeqNum){
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

// Called from layer 3
void A_input(struct pkt packet)
{
    int cs = getchecksum(&packet);
    if (packet.acknum == base && packet.checksum == cs)
    {  
        base=base+1;
        if(base==nextSeqNum){
        	stoptimer(0);
        }
        else{
        	stoptimer(0);
        	starttimer(0, timeout);
        }
        packetList.erase(packetList.begin());
    }
    else
        return;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	starttimer(0, timeout);

	for (int i=0; i < packetList.size(); i++){
		tolayer3(0, packetList[i]);
	}
}

void A_init()
{
	base = 1;
	nextSeqNum = 1;
	N = getwinsize();
}

void B_input(struct pkt packet)
{
    int cs = getchecksum(&packet);

    if (packet.checksum == cs && expectedSeqnumB ==packet.seqnum)
    {
    	// Send message to layer 5
        struct msg message;
        strncpy(message.data, packet.payload, sizeof(message));
        char *messageToSend = message.data;
        tolayer5(1, messageToSend);
               
        // Send ACK to layer 3
        ack.acknum = expectedSeqnumB;
        int acs= getchecksum(&ack);
        ack.checksum=acs;
        tolayer3(1, ack);
        expectedSeqnumB++;
    }
    else{  
        // Send ACK for previous packet
        tolayer3(1, ack);
    }

}

void B_init()
{
 expectedSeqnumB=1;
 ack.acknum=0;
 int acs=getchecksum(&ack);
 ack.checksum=acs;
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