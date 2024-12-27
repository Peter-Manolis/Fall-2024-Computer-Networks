#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "include/rudp.h"
#include "include/sans.h"
#include "include/map.h"

#define SOCKET_SIZE 10
#define MAX_SOCKETS 10
#define PKT_LEN 1400

  struct timeval timeout = {
    .tv_usec = 20000               // 20 milliseconds --> microseconds
  };

  map sm[MAX_SOCKETS];

int sans_connect(const char* host, int port, int protocol) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd;


    if (protocol == IPPROTO_TCP) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        char newPort[6];
        snprintf(newPort, sizeof(newPort), "%d", port);

        int s = getaddrinfo(host, newPort, &hints, &result);
        if (s != 0) {
            return -1;
        }

        for (rp = result; rp != NULL; rp = rp->ai_next) {
            sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sfd == -1) {
                continue;
            }

            if (connect(sfd, rp->ai_addr, rp->ai_addrlen) >= 0) {
                break;
            }
            close(sfd);
        }

        if (rp == NULL) {
            return -1;
        }


        return sfd;
    }


    else if (protocol == IPPROTO_RUDP) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
        hints.ai_flags = AI_PASSIVE;

        char newPort[6];
        snprintf(newPort, sizeof(newPort), "%d", port);

        int s = getaddrinfo(host, newPort, &hints, &result);
        if (s != 0) {
            return -1;
        }

        for (rp = result; rp != NULL; rp = rp->ai_next) {
            sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sfd == -1) {
                continue;
            }

            rudp_packet_t syn = {0};
            syn.type = SYN;

            sendto(sfd, &syn, sizeof(syn), 0, rp->ai_addr, rp->ai_addrlen);

            struct sockaddr_storage address;
            socklen_t addressSize = sizeof(address);
            rudp_packet_t synACK = {0};


            setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

            while (1) {
                if (recvfrom(sfd, &synACK, sizeof(synACK), 0, (struct sockaddr *) &address, &addressSize) == -1) {
                    sendto(sfd, &syn, sizeof(syn), 0, rp->ai_addr, rp->ai_addrlen);
                } else if (synACK.type == (SYN | ACK)) {
                    break;
                }
            }

            rudp_packet_t ack = {0};
            ack.type = ACK;
            sendto(sfd, &ack, sizeof(ack), 0, (struct sockaddr *) &address, addressSize);
            break;
        }

         return sfd;
    }

    return -1;
}


int sans_accept(const char* addr, int port, int protocol) {

  
  struct addrinfo hints;
  struct addrinfo *result;
  struct addrinfo *rp;
  struct sockaddr_storage client_addr;
  int sfd;


  if(protocol == IPPROTO_TCP){ 
    memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_next = NULL;

  char newPort[6];
  snprintf(newPort, sizeof(newPort), "%d", port);
  int s = getaddrinfo(addr, newPort, &hints, &result);
  
  if (s == -1){
    return -1;
  }
  
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1){
      continue;
    }
    if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
      
      if (listen(sfd, SOMAXCONN) == 0){
	  break;
      }
    }
    
    close(sfd);
  }
  if (rp == NULL) {
        return -1;
  }
  socklen_t clientLen = sizeof(client_addr);
  
  int cfd = accept(sfd, (struct sockaddr *)&client_addr, &clientLen);
  if (cfd == -1) {
        close(sfd);
        return -1;
    }
    return cfd;
  }
  else if(protocol == IPPROTO_RUDP){
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    
    hints.ai_socktype = SOCK_DGRAM; 
    hints.ai_flags = AI_PASSIVE;    
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    char newPort[6];

    snprintf(newPort, sizeof(newPort), "%d", port);

    int s = getaddrinfo(addr, newPort, &hints, &result);

    for (rp = result; rp != NULL; rp = rp->ai_next) {
      s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (s != -1)
        break;
    }


    rudp_packet_t rcv = {0};
    rudp_packet_t sn = {0};
    map pack;
    socklen_t addrLen = sizeof(rp->ai_addrlen);
    recvfrom(s, &rcv, PKT_LEN, 0, rp->ai_addr, &addrLen);
    while(1){
      if(rcv.type == SYN){
        break;
      }
      recvfrom(s, &rcv, PKT_LEN, 0, rp->ai_addr, &addrLen);
    }
    

    pack.sock = s;
    pack.host.ai_addr = rp->ai_addr;
    sm[s] = pack;

    sn.type = SYN | ACK;
    sendto(s, &sn, PKT_LEN, 0, rp->ai_addr, rp->ai_addrlen);
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    while(1){
      if(recvfrom(s, &rcv, PKT_LEN, 0 , rp->ai_addr, &addrLen) == -1){
        sn.type = SYN | ACK;
        sendto(s, &sn, PKT_LEN, 0, rp->ai_addr, rp->ai_addrlen);
      }
      else 
        break;
    }
  
    
    return s;
  }  
  return 0;
}


int sans_disconnect(int socket) {
    return close(socket);
}
