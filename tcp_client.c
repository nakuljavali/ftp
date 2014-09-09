#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "customhead.h"
#include "log.h"
#include "macro_tcp.h"

pthread_t tcp_client;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
struct mytcpheader head;


void *arq_send(void *args){

	int sockfd, n, i, count;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int arr[MAX_ARQ_SIZE/16] = {0};


	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		LOGERR("ERROR opening socket");

	server = gethostbyname(argv[1]);
	if (server == NULL) {
		LOGERR("ERROR, no such host\n");
		exit(0); 
		}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(TCP_PORT);


	pthread_mutex_lock(&lock);

	arr = head.nack_array;
 
	pthread_mutex_unlock(&lock);

	count = 0;
	for(i =0, i<sizeof(arr), i++){
		if (arr[i] == 0)
			count++;
	}

	usleep(count* PACKET_TIME);

	for(i =0, i<sizeof(arr), i++){
		if (arr[i] == 0)
			count++;
	}

	if(0 != count){

		if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
			LOGERR("ERROR connecting");
		
		n = send(sd, (void*)&arr, sizeof(arr), 0);
		if (n < 0) 
			LOGERR("ERROR writing to socket");
	}

	close(sockfd);

}

int main(){


	char *msg = "tcp_client";
	

	if(pthread_create(&tcp_client, NULL, arq_send, (void*)msg)) {
		LOGERR("Error creating thread\n");	
		return 1;
		}

		pthread_join(tcp_client);
		return 0;


}