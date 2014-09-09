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
#include <pthread.h>

#include "log.h"
#include "macros.h"

#define PROTOPORT 7654

void *tcp_server()
{

    int sock, connected, bytes_recieved , true = 1;  
    char send_data [1024] , recv_data[1024];       

    struct sockaddr_in server_addr,client_addr;    
    int sin_size, element, n = 1;
        
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("Socket");
      exit(1);
    }

    if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
      perror("Setsockopt");
      exit(1);
    }
        
    server_addr.sin_family = AF_INET;         
    server_addr.sin_port = htons(TCP_PORT);     
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    bzero(&(server_addr.sin_zero),8); 

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))
	== -1) {
      perror("Unable to bind");
      exit(1);
    }

    if (listen(sock, 5) == -1) {
      perror("Listen");
      exit(1);
    }

    sin_size = sizeof(struct sockaddr_in);

    connected = accept(sock, (struct sockaddr *)&client_addr,&sin_size);

    printf("\n I got a connection from (%s , %d)",
	       inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

    while (1)
    {

        bytes_recieved = recv(connected,recv_data,1024,0);

	recv_data[bytes_recieved] = '\0';
    
        for (element = 0; element < TCP_ARRAY_SIZE/8; element++) {
	    if (recv_data[element] & 0b10000000 == 128)
	        send_UDP(n*element)
	    if (recv_data[element] & 0b01000000 == 64)
                send_UDP(n*element+1);  
	    if (recv_data[element] & 0b00100000 == 32)
                send_UDP(n*element+2);
	    if (recv_data[element] & 0b00010000 == 16)
                send_UDP(n*element+3);
	    if (recv_data[element] & 0b00001000 == 8)
                send_UDP(n*element+4);
	    if (recv_data[element] & 0b00000100 == 4)
                send_UDP(n*element+5);
	    if (recv_data[element] & 0b00000010 == 2)
                send_UDP(n*element+6);
	    if (recv_data[element] & 0b00000001 == 1)
                send_UDP(n*element+7);

            n += 8;
         }
    }

    close(sock);
    return 0;

}





int main()
{

int sock;
struct sockaddr_in server_addr;
struct hostent *host;
char buffer[1024];

pthread_t tcp_thread;

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

if(pthread_create(&tcp_thread, NULL, tcp_server, NULL)) {

   fprintf(stderr, "Error creating thread\n");
   return 1;

}



 while ( read_bytes = fread(buffer, sizeof(buffer), 1, fp) != 0) {
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
