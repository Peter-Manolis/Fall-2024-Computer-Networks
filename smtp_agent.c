#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "include/sans.h"

int correct_response(const char *response){
  if(strncmp(response, "250", 3) == 0){
    return 1;
  }
  else {
    return 0;
  }
	
}

int smtp_agent(const char* host, int port) {
  
  char receiver[32];
  char email_body[32];

  int sock_id = sans_connect(host, port, IPPROTO_TCP);

  if(scanf("%[^\n]%*c", receiver) <= 0){
    sans_disconnect(sock_id);
    return -1;
  }
  if(scanf("%[^\n]%*c", email_body) <= 0){
    sans_disconnect(sock_id);
    return -1;
  }

  char buf[64];
  sans_recv_pkt(sock_id, buf, 64);
  if(strncmp(buf, "220", 3) != 0){
    sans_disconnect(sock_id);
    return -1;
  }

  char buf2[128];
  snprintf(buf2, 128, "HELO %s", receiver);
  sans_send_pkt(sock_id, buf2, strlen(buf2));

  char buf3[64];
  sans_recv_pkt(sock_id, buf3, 63);
  if(correct_response(buf3)== 0){
    sans_disconnect(sock_id);
    return -1;
  }

  char buf4[128];
  snprintf(buf4, 128, "MAIL FROM: %s", receiver);
  sans_send_pkt(sock_id, buf4, strlen(buf4));

  char buf5[64];
  sans_recv_pkt(sock_id, buf5, 63);
  if(correct_response(buf5)== 0){
    sans_disconnect(sock_id);
    return -1;
  }
  
  char buf6[64];
  snprintf(buf6, 64, "RCPT TO: %s", receiver);
  sans_send_pkt(sock_id, buf6, strlen(buf6));

  char buf7[64];
  sans_recv_pkt(sock_id, buf7, 63);
  if(correct_response(buf7)== 0){
    sans_disconnect(sock_id);
    return -1;
  }
  char buf10[64];
  snprintf(buf10, sizeof(buf10),
	   "DATA\r\n");
  sans_send_pkt(sock_id, buf10, strlen(buf10));

  char buf9[256];
  sans_recv_pkt(sock_id, buf9, 255);

  if(strncmp(buf9, "354", 3) !=0){
    sans_disconnect(sock_id);
    return -1;
  }

  char fileBuf[1024];
  FILE *file = fopen(email_body, "r");
  size_t chunks;
  while((chunks = fread(fileBuf, 1, sizeof(fileBuf), file)) > 0){
    sans_send_pkt(sock_id, fileBuf, chunks);
  }
  fclose(file);

  char buf11[64];
  snprintf(buf11, sizeof(buf11),
	     "\r\n.\r\n");
  
  sans_send_pkt(sock_id, buf11, strlen(buf11));


  char end[1024];
  sans_recv_pkt(sock_id, end, 1024);


  char buf12[64];
  snprintf(buf12, sizeof(buf12),
	  "QUIT\r\n");
  sans_send_pkt(sock_id, buf12, strlen(buf12));

  char buf8[64];
  sans_recv_pkt(sock_id, buf8, 63);
  if(strncmp(buf8, "221", 3) != 0){
    sans_disconnect(sock_id);
    return -1;
  }

  sans_disconnect(sock_id);
  return 0;
}
