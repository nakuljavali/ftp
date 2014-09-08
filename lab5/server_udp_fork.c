/* Sample UDP server */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include "packet_header.h"


// function applied on the received message 
void do_stuff(void *);



int main(int argc, char**argv)
{
   int sockfd,n=0;
   struct sockaddr_in servaddr,cliaddr;
   socklen_t len;
   char mesg[1000];
   char buffer[256];
   
   strcpy(buffer,"I have nothing to send");

   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
   servaddr.sin_port=htons(8000);
   bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

   for (;;)
   {
      len = sizeof(cliaddr);
	  
      n = recvfrom(sockfd,mesg,sizeof(struct udpheader),0,(struct sockaddr *)&cliaddr,&len);
	  
      sendto(sockfd,buffer,sizeof(buffer),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
	  printf("\n Recevied n= %dBytes",n);
      printf("-------------------------------------------------------\n");
      //mesg[n] = 0;
      printf("Received the following:\n");
	  do_stuff((void *)mesg);
      printf("-------------------------------------------------------\n");
   }
}

	void do_stuff(void *mesg)
	{
	struct udpheader *udp = (struct udpheader *) (mesg);
	  /*
	  udp->udph_srcport = 8000;
	udp->udph_destport = 9000;
	udp->udph_len = 1000;
	udp->udph_chksum = 1;*/
	//printf("\n Size of the message is %ld",sizeof(mesg));
	printf("\nudp->udph_srcport = %d",udp->udph_srcport);
	printf("\nudp->udph_destport = %d",udp->udph_destport);
	printf("\nudp->udph_len = %d",udp->udph_len);
	printf("\nudp->udph_chksum = %d",udp->udph_chksum);
	printf("\nudp->data = %s",udp->data);

	
	//char *data = (char *)((void *)mesg + sizeof(struct udpheader));
	//printf("\n Data = %s",data);
	
	printf("\n");
	
	}