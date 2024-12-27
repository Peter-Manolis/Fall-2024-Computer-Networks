#ifndef RUDP_H
#define RUDP_H

typedef struct {
    int type;
    int seqnum;
    char payload[]; // Flexible array member for payload
} rudp_packet_t;

#endif // RUDP_H




#define DAT 0
#define SYN 1
#define ACK 2
#define FIN 4
/*
#define MAXSIZE 1400
typedef struct {
  char type;
  int seqnum;
  char payload[];
} rudp_packet_t;

*/
