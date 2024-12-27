#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/sans.h"

void get_response(int sock) {
  int content_len = 0;
  int in_headers = 1;
  int lookfor = 0;
  do {
    char response[1024] = {0};
    int len = sans_recv_pkt(sock, response, 1024);
    for (int i = 0; i < len; i++) {
      if (lookfor == 0 && response[i] == '\r')
        lookfor = 1; 
      else if (lookfor == 1 && response[i] == '\n') {
        lookfor = 2; 
        if (strncmp(&(response[i + 1]), "Content-Length", 14) == 0) {
          content_len = strtol(&(response[i + 16]), NULL, 0);
        }
      } else if (lookfor == 2 && response[i] == '\r') 
        lookfor = 3; 
        else if (lookfor == 3 && response[i] == '\n') { 
        in_headers = 0;
      } else
        lookfor = 0;
      
      printf("%c", response[i]);

      if (!in_headers && content_len != 0)
        content_len -= 1;
    }
  } while (in_headers);
  char response[1024] = {0};

  while ((content_len = sans_recv_pkt(sock, response, sizeof(response)-1))>0){
    response[content_len] = '\0';
    printf("%s", response);
  }
  /*
  while (content_len > 0) {
    char response[1024] = {0};
    content_len -= sans_recv_pkt(sock, response, 1024);
    printf("%s", response);
    
  }
  */
}

int http_client(const char* host, int port) {
  int sock, len;
  char method[8], path[512];
  char request[1024] = {0};
  scanf("%8s %512s", method, path);
  len = snprintf(request, sizeof(request),
		 "%s /%s HTTP/1.1\r\nHost:%s:%d\r\n"
		 "User-Agent: sans/1.0\r\n"
		 "Cache-Control: no-cache\r\n"
		 "Connection: close\r\n"
		 "Accept: */*\r\n\r\n", method, path, host, port);
  if ((sock = sans_connect(host, port, IPPROTO_TCP)) < 0) {
    printf("[ERROR] Unable to connect to host `%s:%d`\n", host, port);
    return -1;
  }
  sans_send_pkt(sock, request, len + 1);
  get_response(sock);
  sans_disconnect(sock);
  return 0;
}

