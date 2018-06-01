#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <vector>
using namespace std;

/* ******************************************************************

ALTERNATING-BIT NETWORK EMULATOR: VERSION 1.1 

**********************************************************************/

static int waitingOnACK;      // 0 : Ready to send, 1: Waiting on ACK
static int seqnum_ = 0;
static struct pkt packet_a;
static std::vector <msg> messageList;
int expectedSeqnumB;
int checksum = 0;
float timeout = 25.0;

int getchecksum(struct pkt *packet);
int flip_number(int to_flip);

/* called from layer 5 */
void A_output(struct msg message)
{
    if (waitingOnACK==0)
    {
        messageList.push_back(message);
 
        // build the packet
        struct msg msgToSend=(struct msg)messageList.front();
        packet_a.seqnum = seqnum_;
        packet_a.acknum = -1;
        strncpy(packet_a.payload, msgToSend.data, sizeof(message));
        int cs = getchecksum(&packet_a);
        packet_a.checksum = cs;

        // update waiting state
        waitingOnACK = 1;

        // send packet & start timer
        tolayer3(0, packet_a);
        starttimer(0, timeout);
    }
    else{
        messageList.push_back(message);
        return;
    }
}

/* called from layer 3 */
void A_input(struct pkt packet)
{   
    int cs = getchecksum(&packet);
    if (packet.acknum == seqnum_ && packet.checksum == cs)
    {
        stoptimer(0);
        waitingOnACK=0;
        seqnum_=flip_number(seqnum_);
        messageList.erase(messageList.begin());
    }
    else
        return;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    tolayer3(0, packet_a);
    starttimer(0, timeout);
}  

void A_init()
{
    seqnum_ = 0;
    waitingOnACK = 0;
}

void B_input(struct pkt packet)
{
    int cs = getchecksum(&packet);
    if (packet.checksum != cs || expectedSeqnumB !=packet.seqnum){
        // Packet transmission failed, corrupted or incorrect sequence);
        struct pkt ack_b;
        ack_b.acknum = flip_number(expectedSeqnumB);
        int acs= getchecksum(&ack_b);
        ack_b.checksum=acs;
        tolayer3(1, ack_b);
    }
    else{
        // Packet transmission successful
        struct msg message;
        strncpy(message.data, packet.payload, sizeof(message));
        char *messageToSend = message.data;
        tolayer5(1, messageToSend);
        expectedSeqnumB=flip_number(expectedSeqnumB);
        struct pkt ack_b;
        ack_b.acknum = packet.seqnum;
        int acs= getchecksum(&ack_b);
        ack_b.checksum=acs;
        tolayer3(1, ack_b);
    }
}

void B_init()
{
    expectedSeqnumB = 0;
}

int flip_number(int to_flip){ return (to_flip == 0) ? 1 : 0;}

int getchecksum(struct pkt *packet)
{
    char msg[20];
    strncpy(msg, packet->payload, sizeof(msg));
    int checksum = 0;
    
    if(packet){
        for (int i=0; i<20; i++)
            checksum += (unsigned char)packet->payload[i];
        checksum += packet->seqnum;
        checksum += packet->acknum;
        return checksum;
    }
    else 
        return 0;
}


