#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <assert.h>
#include "packet_header.h"


void error(const char *msg)
{
    perror(msg);
    exit(0);
}



int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr , cli_addr;
    struct hostent *server;
	socklen_t clilen;


    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    //if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
      //  error("ERROR connecting");
    //printf("Please enter the message: ");
    bzero(buffer,256);
    //fgets(buffer,255,stdin);
	/*
	 unsigned short int udph_srcport;
 unsigned short int udph_destport;
 unsigned short int udph_len;
 unsigned short int udph_chksum;
	*/
	
	//struct udpheader *udp = (struct udpheader *) (buffer);
	
	
	//void *datagram = (void *)malloc(16);
   // printf("\n Size of datagram = %ld\n",sizeof(datagram));


	
	printf("\n Size of UDP header = %ld\n",sizeof(struct udpheader));
	struct udpheader *udp = (struct udpheader *) malloc(sizeof(struct udpheader));
	assert(udp != NULL);

	//struct udpheader *udp = (struct udpheader *) datagram;
	udp->udph_srcport = 8555;
	udp->udph_destport = 990;
	udp->udph_len = 1000;
	udp->udph_chksum = 6;
	strcpy(udp->data,"niranjan");
	


	//char *msg = (char *)((void*)datagram+sizeof(struct udpheader));
	//strcpy(msg,"niranjan");
	
	//printf("\n Embedded message is: %s and size = %ld\n",(char *)((void*)datagram+sizeof(struct udpheader)),sizeof(datagram));
	
    //n = write(sockfd,buffer,strlen(buffer));
	n = sendto(sockfd,udp,sizeof(struct udpheader),0,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
	//printf("\nSent message : %d",(struct udpheader *)buffer->udph_srcport);

    if (n < 0) 
         error("ERROR writing to socket");
    //bzero(buffer,256);
    
	//n = read(sockfd,buffer,255);
	n = recvfrom(sockfd,buffer,sizeof(buffer),0,(struct sockaddr *)&cli_addr,&clilen);

    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);
    
	close(sockfd);
    return 0;
}

