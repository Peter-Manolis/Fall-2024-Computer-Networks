#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "include/rudp.h"
#include "sans_backend.h"
#include <unistd.h>
#define SWND_SIZE 10

typedef struct {
  int socket;
  int packetlen;
  rudp_packet_t* packet;
} swnd_entry_t;

swnd_entry_t send_window[SWND_SIZE] = {0};  /*  Ring buffer containing the packets awaiting acknowledgement */

static int sent = 0;
static int count = 0;
static int head = 0;



/*
 * Helper: We recommend writing a helper function for pushing a packet onto the ring buffer
 *         that can be called from the main thread.
 *    (Remember to copy the packet to avoid thread conflicts!)
 */


void enqueue_packet(int sock, rudp_packet_t* pkt, int len) {
  if (count == SWND_SIZE) {
        return;
    }

    int tail = (head + count) % SWND_SIZE;
    send_window[tail].socket = sock;
    send_window[tail].packetlen = len;
    send_window[tail].packet = malloc(sizeof(rudp_packet_t) + len);
    if (send_window[tail].packet == NULL) {
        return;
    }
    memcpy(send_window[tail].packet, pkt, sizeof(rudp_packet_t) + len);

    count++;

    return;


}



/*
 * Helper: We recommend writing a helper function for removing a completed packet from the ring
 *         buffer that can be called from the backend thread.
 */
static void dequeue_packet(void) {
  if (count == 0){
    return;
  }
  free(send_window[head].packet);
  send_window[head].packet = NULL;
  head = (head + 1) % SWND_SIZE;
  count--;
  sent--;
}

/*  Asynchronous runner for the sans protocol driver.  This function  *
 *  is responsible for checking for packets in the `send_window` and  *
 *  handling the transmission of these packets in a reliable manner.  *
 *  (i.e. Stop-and-Wait or Go-Back-N)                                 */


void* sans_backend(void* unused) {
  while (1) {
    
    while (sent < count) {
      int index = (sent + head) % SWND_SIZE;
      swnd_entry_t entry = send_window[index];

      
      if (sendto(entry.socket, entry.packet, entry.packetlen, 0, NULL, 0) == -1) {
	break;
      }
      sent++;
    }
    
    if (count > 0) {
      rudp_packet_t answer;
      int recVal = recvfrom(send_window[head].socket, &answer, sizeof(rudp_packet_t), 0, NULL, NULL);
      if (recVal > 0) {
        if (answer.type == ACK) {
          if (answer.seqnum == send_window[head].packet->seqnum) {
            dequeue_packet();
          }
        }
      
      } else {
        sent = 0;
      }
    }
    usleep(500);

  }
  /*----------------------------------*/
  return NULL;
}


