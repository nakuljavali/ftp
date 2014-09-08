#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
 
int main (void)
{

  int s = socket (AF_INET, SOCK_RAW, IPPROTO_UDP);
     
  if(s == -1)
    {
      perror("Failed to create raw socket");
      exit(1);
    }
  char buffer[256];
  
  printf("waiting to receive some data ....\n");
  
  int count = 0;

  while ((count = read (s, buffer, 256)) > 0) {
    fwrite(buffer+sizeof(struct iphdr)+sizeof(struct udphdr), count-(sizeof(struct iphdr)+sizeof(struct udphdr)), 1, stdout);
    printf("\n");
  }
  return 0;
}
