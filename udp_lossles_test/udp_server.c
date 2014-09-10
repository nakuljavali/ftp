/* udpserver.c */ 

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "macros.h"

#include "customhead.h"

//tcp client
#include "log.h"
#include <pthread.h>

pthread_t tcp_client;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
struct mytcpheader head;


// thread for tcp client

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

     if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        LOGERR("ERROR connecting");

while(1){
    pthread_mutex_lock(&lock);

    arr = head.nack_array;
 
    pthread_mutex_unlock(&lock);

    count = 0;
    for(i =0, i<sizeof(arr), i++){
        if (arr[i] == 0)
            count++;
    }

    if (0 == count)
        pthread_exit(0);

    usleep(count* PACKET_TIME);

    pthread_mutex_lock(&lock);

    arr = head.nack_array;
 
    pthread_mutex_unlock(&lock);


    n = send(sd, (void*)&arr, sizeof(arr), 0);
        if (n < 0) 
            LOGERR("ERROR writing to socket");
    
}
    close(sockfd);

}

int main()
{

    int sock;
    socklen_t addr_len;
    int  bytes_read;
    char recv_data[sizeof(struct myudpheader)];
    struct sockaddr_in server_addr , client_addr;


    if(pthread_create(&tcp_client, NULL, arq_send, "tcp_client") {
        LOGERR("Error creating thread\n");  
        return 1;
        }



        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(UDP_PORT);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_addr.sin_zero),8);


        if (bind(sock,(struct sockaddr *)&server_addr,
            sizeof(struct sockaddr)) == -1)
        {
            perror("Bind");
            exit(1);
        }

        addr_len = sizeof(struct sockaddr);
		
	printf("\nUDPServer Waiting for client on port 5000");
        fflush(stdout);
	
	FILE *fp = fopen("output_nodeb.bin","w");
	assert(fp != NULL);
	char *heap_mem = (char *)malloc((MAX_DATA_SIZE*MAX_DATA_SIZE));
	while (1)
	{

          bytes_read = recvfrom(sock,recv_data,sizeof(struct myudpheader),0,(struct sockaddr *)&client_addr, &addr_len);
	  

          struct myudpheader* recv_payload = (struct myudpheader*)(recv_data);

//          printf("\n seq_no received =%d", recv_payload->sequence_no);
//	  fflush(stdout);          
//printf("\n data =");
	memcpy(heap_mem+(recv_payload->sequence_no)*MAX_DATA_SIZE,recv_payload->payload_data,MAX_DATA_SIZE);


    head.nack_array[recv_payload->sequence_no] = 1;
    
	if(recv_payload->sequence_no == MAX_DATA_SIZE-1)
	 {
	  printf("\n Writing to file\n");
	  fwrite(heap_mem,MAX_DATA_SIZE,MAX_DATA_SIZE,fp);
	  fclose(fp);
	  exit(0);
	}
//	  recv_data[bytes_read] = '\0';

//          printf("\n(%s , %d) said : ",inet_ntoa(client_addr.sin_addr),
  //                                     ntohs(client_addr.sin_port));

        }
        return 0;
}
