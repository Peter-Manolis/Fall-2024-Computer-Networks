#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SOCKET_SIZE 10

typedef struct map{
    int sock;
    struct addrinfo host;
  int seqnum;
} map;

extern map sm[SOCKET_SIZE];
