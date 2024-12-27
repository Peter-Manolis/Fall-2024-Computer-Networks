#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include "include/map.h"
#include "include/rudp.h"


#define DAT 0
#define SYN 1
#define ACK 2
#define FIN 4

#define MAXSIZE 1400

extern void enqueue_packet(int sock, rudp_packet_t* pkt, int len);

rudp_packet_t last_good_packet;  
int global_seqnum = 0;
int seqnum = 0;



int sans_send_pkt(int socket, const char* buf, int len) {
    if (len > MAXSIZE) {
        return -1;
    }

    rudp_packet_t *packet = malloc(sizeof(rudp_packet_t) + len);
    if (!packet) {
        return -1;
    }


    packet->type = DAT;
    packet->seqnum = global_seqnum;
    memcpy(packet->payload, buf, len);
    packet->payload[len] = '\0'; 

    enqueue_packet(socket, packet, len);
    free(packet);
    global_seqnum++; 

    return 0;
}


int sans_recv_pkt(int socket, char* buf, int len) {
    map s = sm[socket];
    rudp_packet_t packet = {0};
    rudp_packet_t ack_packet = {0};
    struct sockaddr_storage address;
    socklen_t addr_len = sizeof(address);

    while (1) {

        if (recvfrom(socket, &packet, sizeof(rudp_packet_t) + len, 0, (struct sockaddr *)&address, &addr_len) == -1) {

            return -1;
        }


        if (packet.type == DAT && packet.seqnum == s.seqnum) {

            memcpy(buf, packet.payload, len);


            ack_packet.type = ACK;
            ack_packet.seqnum = s.seqnum;
            sendto(socket, &ack_packet, sizeof(rudp_packet_t), 0, (struct sockaddr *)&address, addr_len);


            s.seqnum++;
            sm[socket] = s;
            return len;
        } else {

            ack_packet.type = ACK;
            ack_packet.seqnum = s.seqnum - 1;
            sendto(socket, &ack_packet, sizeof(rudp_packet_t), 0, (struct sockaddr *)&address, addr_len);
        }
    }
}
