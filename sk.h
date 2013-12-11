#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
typedef struct
{
  struct sockaddr_in sin;
  int sinlen;
  int bindflag;
  int sd;
  char ip[16];
  short port;
}skt_s;
skt_s *tconnect(char *,short,int);
int tsend(skt_s *,char *,int,int);
