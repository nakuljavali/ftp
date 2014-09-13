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
#include <errno.h>

#include "../macros.h"
#include "../customhead.h"
#include "../log.h"

#include "../myparameters.c"


pthread_t tcp_client,ack_server_thread;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int stop_flag = 0;
int current_batch = 0;
char *nack_pointer = NULL;
int batch_set_flag = 0;
int create_array_flag =0;
int tcp_flag = 0;

int range[5]={0,0,0,0,0};

void *ack_thread(void *args) {
  int acksock,true;
    socklen_t addr_len;
    int bytes_read = 0;
    struct hostent *host;

    int num = 0;

    struct sockaddr_in ack_server;

    host= (struct hostent *) gethostbyname((char*)"10.1.1.2");

    if((acksock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        LOGERR("ERROR creating UDP socket\n");
        exit(1);
    }

    ack_server.sin_family = AF_INET;
    ack_server.sin_port = htons(ACK_PORT);
    ack_server.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(ack_server.sin_zero),8);



  if (setsockopt(acksock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
      perror("Setsockopt");
      LOGERR("Client/TCP/Socket/Reuseaddr");
      exit(1);
  }

  /*    if (bind(acksock,(struct sockaddr *)&ack_server,sizeof(struct sockaddr)) == -1){
        perror("socket error");
	printf("%s\n", strerror(errno));
        LOGERR("ERROR binding UDP socket\n");
        exit(1);
    }
  */
  printf("ack client\n");

    while (1) {
        sendto(acksock,(void *)&num, sizeof(int) , 0,(struct sockaddr *)&ack_server, sizeof(struct sockaddr));
        num++;
	//        printf("%d\n",num);
        usleep(10000);

      printf("%d\n",num);

    }


}

int print_array_count(char arr[batch_size]){
    int i,count = 0;
    for(i = 0; i < batch_size; i++)
        if (arr[i]=='0')
            count++;

    //    printf("COUNT: %d",count);
    //    printf(",current batch: %d\n",current_batch);
    fflush(stdout);
    
    return count;
}

void *tcp_thread(void *args){

    int sockfd, n, i, count;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct infoheader info;

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

    // TCP reading the first info header

    
    n = read(sockfd,(void*)&info, sizeof(info));
    if (n < 0) 
        LOGERR("ERROR reading from socket\n");



    packet_size = info.size_pkt;
    filesize = info.size_file;
    no_of_packets = info.no_packets;
    batch_size = info.size_batch;
    no_of_batches = info.no_batches;
    last_batch_size = info.size_last_batch;
    last_packet_size = info.size_last_packet;
    batch_set_flag = 1;

    LOGDBG("packtet size : %d\n", packet_size);
    LOGDBG("file size : %d\n", filesize );
    LOGDBG("batch size : %d\n", batch_size);
    LOGDBG("no_batches : %d\n", no_of_batches);
    LOGDBG("no_packets : %d\n", no_of_packets);
    LOGDBG("size_last_batch : %d\n", last_batch_size);
    LOGDBG("size_last_packet : %d\n", last_packet_size);

    LOGDBG("Received the info header\n");

    while(!create_array_flag);
    char nack_array[batch_size];
    char arr[batch_size];
    nack_pointer = nack_array;

    for (i = 0; i < batch_size; i++) {
        arr[i] = '0';
        nack_array[i]= '0';
    }


    print_array_count(nack_pointer);

    
    while(1){

      //        pthread_mutex_lock(&lock);
        memcpy(&arr,nack_pointer,batch_size);
	//pthread_mutex_unlock(&lock);
        print_array_count(nack_pointer);

        usleep(1000);

        memcpy(&arr,nack_pointer,batch_size);
        count = 0;
        for(i =0; i< batch_size; i++){
            if (arr[i]=='0')
                count++;
        }

        if (range[0]==0 && count>1000){
            n = send(sockfd, (void*)&arr, sizeof(arr), 0);
            range[0]=1;
            printf("sending count %d\n",count);
        } else if (range[1]==0 && count>500 && count <500){
            n = send(sockfd, (void*)&arr, sizeof(arr), 0);
            range[1]=1;
            printf("sending count %d\n",count);
        } else if (range[2]==0 && count>200 && count < 500) {
            n = send(sockfd, (void*)&arr, sizeof(arr), 0);
            range[2]=1;
            printf("sending count %d\n",count);
	} else if (range[3] == 0 && count >0 && count < 200) {
            n = send(sockfd, (void*)&arr, sizeof(arr), 0);
            range[3]=1;
            printf("sending count %d\n",count);
        } else if (range[4] == 0 && count == 0) {
            n = send(sockfd, (void*)&arr, sizeof(arr), 0);
            range[2] = 0;
            range[0] = 0;
            range[1] = 0;
            range[3] = 0;
            range[4] = 0;

            printf("sending count %d\n",count);
        } else
   	     usleep(10000);


	/*
        if (!tcp_flag && count>200) {usleep(100000); continue;}
        if (count == 8192) tcp_flag = 0;

	//	pthread_mutex_lock(&lock);
        memcpy(&arr,nack_pointer,batch_size);
	//	pthread_mutex_unlock(&lock);

        count = 0;
        for(i =0; i< batch_size; i++){
            if (arr[i]=='0')
                count++;
        }
        printf("count requested %d\n",count);
        print_array_count(arr);

        n = send(sockfd, (void*)&arr, sizeof(arr), 0);

        if (n < 0) {
            LOGERR("ERROR writing to socket\n");
            exit(1);
        }
*/
        if (0 == count){

            current_batch++;
            tcp_flag = 1;
            if(current_batch == no_of_batches){
                stop_flag = 1;
                close(sockfd);
                pthread_exit(0);
            }

            if(current_batch == no_of_batches - 1) batch_size = last_batch_size;
                for (i = 0; i < batch_size; i++) {
                    nack_array[i] = '0';
                        arr[i] = '0';
            }

        }

	}

}

int main(){
    int sock,i;
    socklen_t addr_len;
    int bytes_read = 0;

    struct sockaddr_in server_addr , client_addr;
    struct sockaddr_in ack_server;
    bytes_read = bytes_read;


    if(pthread_create(&tcp_client, NULL, tcp_thread, "tcp_client") != 0){
        LOGERR("ERROR creating TCP thread\n");
        exit(1);
    }
    
    if(pthread_create(&ack_server_thread, NULL, ack_thread, "tcp_client") != 0){
        LOGERR("ERROR creating TCP thread\n");
        exit(1);
    }
    
    while(!batch_set_flag);

    char nack_array[batch_size];
    nack_pointer = nack_array;


    for (i = 0; i < batch_size; i++) {
        nack_array[i] = '0';
    }
    create_array_flag =1;

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

    int count = 0;
    while (1){

        bytes_read = recvfrom(sock,recv_data,packet_size+2, 0,(struct sockaddr *)&client_addr, &addr_len);

	//        printf("cr: %d\n",count++);
      
        fflush(stdout);
        memcpy(&sequence_no,recv_data,2);

        if (current_batch == no_of_batches - 1) {
            batch_size = last_batch_size;
	    if(current_batch%2 == 0){
	      if(sequence_no == last_batch_size - 1)
                packet_size = last_packet_size;
	    }
	    else if(current_batch%2 ==1){
	      if((sequence_no-batch_size) == last_batch_size - 1)
                packet_size = last_packet_size;
	    }
        }


        if(current_batch%2 == 0){
            if(0 <= sequence_no && sequence_no <= batch_size -1){
	        memcpy(heap_mem+(current_batch*batch_size*packet_size)+(packet_size*sequence_no),recv_data+2,packet_size);
                *(nack_pointer+sequence_no) = '1';
            }
        }

        else if(current_batch%2 == 1){
            if(batch_size <= sequence_no &&  sequence_no <= 2*batch_size - 1){
	        memcpy(heap_mem+(current_batch*batch_size*packet_size)+(packet_size*(sequence_no - batch_size)),recv_data+2,packet_size);
                *(nack_pointer+(sequence_no-batch_size)) = '1';   
            }
        }

        
        if(stop_flag){
            printf("Writing to file\n");
            fwrite(heap_mem,1,filesize,fp);
            fclose(fp);
            exit(0);
        }


    }
    return 0;
}
