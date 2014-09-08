#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include "log.h"

#define PROTOPORT 7654

int main()
{

int sock;
struct sockaddr_in server_addr;
struct hostent *host;
char buffer[1024];

host= (struct hostent *) gethostbyname((char *)"nodeA");


if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(PROTOPORT);
server_addr.sin_addr = *((struct in_addr *)host->h_addr);
bzero(&(server_addr.sin_zero),8);

 FILE *fp;
 fp = fopen("hello.txt","r");

 fread(buffer, sizeof(buffer), 1, fp);
 int read_bytes;
 while ( read_bytes = fread(buffer, sizeof(buffer), 1, fp) != 0)
   {
    printf("%s\n",buffer);
    sendto(sock, buffer, sizeof(buffer), 0,
	     (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
    memset(buffer,'\0',sizeof(buffer));
   
   }

 if (strcmp(buffer, "")) {
    printf("%s\n",buffer);
    sendto(sock, buffer, sizeof(buffer), 0,
	     (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
 }

}
