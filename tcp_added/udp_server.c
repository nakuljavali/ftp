/* udp_server.c */

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
#include <netdb.h>
#include <pthread.h>

#include "../macros.h"
#include "../customhead.h"
#include "../log.h"

#include "../myparameters.c"

pthread_t tcp_client;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
struct mytcpheader head = {""};
char arr[batch_size];
int stop_flag = 0;
int current_batch = 0;
char *nack_pointer = NULL;

int print_array_count(char arr[batch_size]){
	int i,count = 0;
    for(i = 0; i < batch_size; i++)
    	if (arr[i]=='0')
    		count++;

    LOGDBG("COUNT: %d\n",count);
    fflush(stdout);
    
    return count;
}

void *tcp_thread(void *args){

    int sockfd, n, i, count;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOGERR("ERROR creating TCP socket\n");
        exit(1);
    }

    server = gethostbyname(CLIENT_IP);
    if (server == NULL) {
        LOGERR("ERROR no such host\n");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(TCP_PORT);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        LOGERR("ERROR connecting to TCP server\n");
        exit(1);
    }

    printf("TCP client connected\n");

    while(1){

    	pthread_mutex_lock(&lock);
    	memcpy(&arr,nack_pointer,batch_size);
    	pthread_mutex_unlock(&lock);

    	count = 0;
    	for(i =0; i<sizeof(arr); i++){
    		if (arr[i]=='0')
    			count++;
    	}

    	usleep(count* PACKET_TIME);

    	pthread_mutex_lock(&lock);
    	memcpy(&arr,nack_pointer,batch_size);
    	pthread_mutex_unlock(&lock);

    	n = send(sockfd, (void*)&arr, sizeof(arr), 0);

        if (n < 0) {
            LOGERR("ERROR writing to socket\n");
            exit(1);
        }

    	if (0 == count){
    		stop_flag = 1;
    		close(sockfd);
    		pthread_exit(0);
    	}

    }
    close(sockfd);

}

int main(){
    int sock,i;
    socklen_t addr_len;
    int bytes_read = 0;

    struct sockaddr_in server_addr , client_addr;
    bytes_read = bytes_read;

    for (i = 0; i < batch_size; i++) {
        head.nack_array[i] = '0';
        arr[i] = '0';
    }


    if(pthread_create(&tcp_client, NULL, tcp_thread, "tcp_client") != 0){
    	LOGERR("ERROR creating TCP thread\n");
        exit(1);
    }


    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    	LOGERR("ERROR creating UDP socket\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero),8);


    if (bind(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
    	LOGERR("ERROR binding UDP socket\n");
        exit(1);
    }

        addr_len = sizeof(struct sockaddr);
		
	printf("UDP Server Waiting for client\n");
        fflush(stdout);
	
	FILE *fp = fopen("/mnt/output_nodeb.bin","w");
	assert(fp != NULL);
	char *heap_mem = (char *)malloc((filesize));


        char recv_data[packet_size+2];
	int sequence_no = 0;

    //printf("starting to receive\n");

	while (1){

		bytes_read = recvfrom(sock,recv_data,packet_size+2, 0,(struct sockaddr *)&client_addr, &addr_len);
	  

		// struct myudpheader* recv_payload = (struct myudpheader*)(recv_data);

        //printf("Seq_no received = %d", recv_payload->sequence_no);
	    //fflush(stdout);

	   
        	memcpy(&sequence_no,recv_data,2);
		memcpy(heap_mem+(current_batch*batch_size)+(packet_size*sequence_no),recv_data+2,packet_size);


		*(nack_pointer+sequence_no) = '1';
    
	    if(stop_flag){
	    	printf("Writing to file\n");
		    fwrite(heap_mem,1,filesize,fp);
		    fclose(fp);
		    exit(0);
	    }


	}
	return 0;
}
