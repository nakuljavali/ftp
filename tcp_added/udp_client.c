#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "../macros.h"
#include "../customhead.h"
#include "../log.h"

#include "../myparameters.c"

#define ONEGIG 8388608

const char *mmaped_file = NULL;
int recv_data;

int sock, bytes_sent = 0;
struct sockaddr_in server_addr;
struct hostent *host;
int start_sending = 0;
int current_batch = 0;
int reset_flag = 0;

char *nack_pointer = NULL;
char *server_ip = NULL;

int latest_count = 0;

/// ACK server parameters

int ack_sock;
struct sockaddr_in ack_server;
socklen_t addr_len;

// dont change this 
int unchanged_batch_size = 0;

int print_array_count(char arr[batch_size])

{

  int i,count = 0;
  if(current_batch == no_of_batches -1)
 	{ for(i = 0; i < last_batch_size; i++)
    		if (arr[i]=='0')
      			count++;
	}
  else
	{
 	   for(i = 0; i < batch_size; i++)
    		if (arr[i]=='0')
      			count++;
	
	}
  return count;

}

void *ini_thread()
{

  int nack_sock ;
  struct sockaddr_in nack_server;

  nack_sock = socket(AF_INET, SOCK_DGRAM, 0);

  nack_server.sin_family = AF_INET;
  nack_server.sin_port = htons(NACK_PORT_SERVER);
  nack_server.sin_addr.s_addr = INADDR_ANY;
  bzero(&(nack_server.sin_zero),8);

  if (bind(nack_sock,(struct sockaddr *)&nack_server,sizeof(struct sockaddr)) == -1){
     LOGERR("ERROR binding UDP socket\n");
     exit(1);
  }

  struct infoheader *info;
  struct infoheader *recv_info;
  int send_count = 0;

  struct timeval tv;
  int recv_bytes;

  //  socklen_t ack_size = sizeof(struct sockaddr);

  tv.tv_sec = 0;
  tv.tv_usec = 100000;

  setsockopt(nack_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

  struct hostent *client;
  client = (struct hostent*) gethostbyname(server_ip);

  struct sockaddr_in nack_client;
  nack_client.sin_family = AF_INET;
  nack_client.sin_port = htons(NACK_PORT_CLIENT);
  nack_client.sin_addr = *((struct in_addr *)client->h_addr);
  bzero(&(nack_client.sin_zero),8);

  info = malloc(sizeof(struct infoheader));
  recv_info = malloc(sizeof(struct infoheader));

  info->sync1 = 65536;
  info->sync2 = 65536;
  info->size_pkt = packet_size;
  info->size_file = filesize;
  info->size_batch = batch_size;
  info->no_batches = no_of_batches;
  info->no_packets = no_of_packets;
  info->size_last_batch = last_batch_size;
  info->size_last_packet = last_packet_size;

  LOGDBG("send_info_packet");
  socklen_t nack_size;
  nack_size = sizeof(struct sockaddr);

  while(1) {

    for (send_count = 0; send_count < 20 ; send_count++) {
      //         LOGDBG("sending info packet");
         if ( sendto(nack_sock,(void*)info,sizeof(struct infoheader),0,(struct sockaddr *)&nack_client,sizeof(struct sockaddr)) == -1)
     	    LOGERR("ERROR: SENDTO: INFO:PKT");
     }

     recv_bytes = recvfrom(nack_sock,(void*)recv_info,sizeof(struct infoheader),0,(struct sockaddr *)&nack_server,&nack_size);

    if(recv_bytes > 0) {
         printf("received bits???\n");
         
         if (recv_info->sync1 == 65536 && recv_info->sync2 == 65536) {
	   //             printf("received info ack!!\n\n\n");
  	     start_sending = 1;
             pthread_exit(0);
         }
    }
 
  }

}


void *ack_thread()
{  
  char recv_data[batch_size+4];
  char temp_buff[batch_size+4]; 
  ack_sock = socket(AF_INET, SOCK_DGRAM, 0);

  ack_server.sin_family = AF_INET;
  ack_server.sin_port = htons(ACK_PORT);
  ack_server.sin_addr.s_addr = INADDR_ANY;
  bzero(&(server_addr.sin_zero),8);

  if (bind(ack_sock,(struct sockaddr *)&ack_server,sizeof(struct sockaddr)) == -1){
    LOGERR("ERROR binding UDP socket\n");
    exit(1);
  }
  addr_len = sizeof(struct sockaddr);


  printf("Ack Server\n");
  int received_batch_no;  
  int i, previous = batch_size;
  while(1) 
    {
      recvfrom(ack_sock,&recv_data,batch_size+4, 0,(struct sockaddr *)&ack_server, &addr_len);
      

      memcpy(&received_batch_no,recv_data,4);
      if(received_batch_no < current_batch)
	continue;

      
      memcpy(temp_buff,recv_data+4,batch_size);



      i = print_array_count(temp_buff);
      //      printf("COUNT :%d, previous count = %d, current batch %d, recv batch no %d, total_batches %d \n",i,previous,current_batch,received_batch_no,no_of_batches);

      if (i == 0) 
	{
	  current_batch++;
                
	  if(current_batch == no_of_batches)
	    {
	      printf("transmission from client side done. Exiting now....\n");
	      exit(1);
	    }

	  if(current_batch == no_of_batches-1)
	    {
	      //	      batch_size = last_batch_size;
	      previous = last_batch_size;
	    } 
	  //	  printf("Batch changed; current batch %d \n",current_batch);
                 
	  previous = batch_size;
	}
      else if(i < previous)
	{
	  previous = i;
          memcpy(nack_pointer,recv_data+4,batch_size);

	}
    }

}


void send_udp(void* mydata)
{
  bytes_sent = sendto(sock,(void *)mydata, (packet_size+2) , 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
}

void send_info_packet()
{
  struct infoheader *info;
  int send_count = 0;

  info = malloc(sizeof(struct infoheader));

  info->sync1 = 65536;
  info->sync2 = 65536;
  info->size_pkt = packet_size;
  info->size_file = filesize;
  info->size_batch = batch_size;
  info->no_batches = no_of_batches;
  info->no_packets = no_of_packets;
  info->size_last_batch = last_batch_size;
  info->size_last_packet = last_packet_size;

  LOGDBG("send_info_packet");

  for (send_count = 0; send_count < 200 ; send_count++) {
    //    LOGDBG("sending info packet");
    if ( sendto(sock,(void*)info,sizeof(struct infoheader),0,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1)
        LOGERR("ERROR: SENDTO: INFO:PKT");
    usleep(50000);
  }
}

int send_by_seq_no(int seq_no)
{
/*
  if(current_batch%2 ==0)
    {if ( (current_batch == no_of_batches-1) && (seq_no == last_batch_size-1) ){

	packet_size=last_packet_size;

      }
    }
  else if(current_batch%2 == 1)
    {

      if ( (current_batch == no_of_batches-1) && (seq_no-batch_size == last_batch_size-1) ){
	packet_size=last_packet_size;
      }}
*/
  char* myudp = (char *) malloc(packet_size+2);
   
  memcpy(myudp,&seq_no,2);

  if(current_batch%2 == 0){

    memcpy(myudp+2,mmaped_file+(current_batch*unchanged_batch_size*packet_size)+(seq_no*packet_size),packet_size);

  }
  else if(current_batch%2 == 1) {
    memcpy(myudp+2,mmaped_file+(current_batch*unchanged_batch_size*packet_size)+((seq_no-unchanged_batch_size)*packet_size),packet_size);

  }
  bytes_sent = sendto(sock,(void *)myudp, (packet_size+2) , 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
       

  free(myudp); 
  return 0; 
}

const char *read_file_to_heap(char *file_name)
{
  const char *memblock;
  int fd;
  struct stat sb;

  fd = open(file_name, O_RDONLY);
  if (fd == -1) {
    LOGERR("Client/Fopen/file error");
    exit(1);
  }

  fstat(fd, &sb);
  LOGDBG("Size: %lu\n", (uint64_t)sb.st_size);
  memblock = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
 // memblock = mmap(NULL, no_of_packets*packet_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
  
 
  if (memblock == MAP_FAILED) {
    LOGERR("Client/Mmap");
  }
  LOGDBG("Mmap SUCCESSFULL.........");
  close(fd);

  return memblock;
}

void *tcp_server()
{
  int sock, connected, true;
  char recv_data[batch_size];

  struct sockaddr_in server_addr,client_addr;
  socklen_t sin_size;

  int i,bytes_recv;
  struct infoheader *info;

  for(i = 0; i < batch_size; i++)
    recv_data[i] = '0';

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    LOGERR("Client/TCP/Socket/Create");
    exit(1);
  }

  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
    perror("Setsockopt");
    LOGERR("Client/TCP/Socket/Reuseaddr");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(TCP_PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  bzero(&(server_addr.sin_zero),8);
  sin_size = sizeof(struct sockaddr_in);

  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    perror("Unable to bind");
    LOGERR("Client/TCP/Socket/Bind\n");
    exit(1);
  }

  if (listen(sock, 2) == -1) {
    perror("Listen");
    LOGERR("Client/TCP/Socket/Listen\n");
    exit(1);
  }

  if ( (connected = accept(sock, (struct sockaddr *)&client_addr,&sin_size))==-1) {
    perror("Listen");
    LOGERR("Client/TCP/Socket/Connected\n");
    exit(1);
  }

  LOGDBG("I got a connection from (%s , %d)",
	 inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
  
  info = malloc(sizeof(struct infoheader));
  info->size_pkt = packet_size;
  info->size_file = filesize;
  info->size_batch = batch_size;
  info->no_batches = no_of_batches;
  info->no_packets = no_of_packets;
  info->size_last_batch = last_batch_size;
  info->size_last_packet = last_packet_size;

  send(connected,(void*)info,sizeof(struct infoheader), 0);
  LOGDBG("Info Packet Sent");
  pthread_exit(0) ; 
  start_sending = 1;

  LOGDBG("Listening on TCP Port, to receive nack data");

  while(0) {
      bytes_recv = recv(connected,recv_data,batch_size,MSG_WAITALL);
      if (bytes_recv > 0) {
  //           LOGDBG("Bytes RECEIVED %d",bytes_recv);
           memcpy(nack_pointer,recv_data,batch_size);

           int i;
           i = print_array_count(recv_data);
           printf("ARRAY RECEIVED OF COUNT %d\n",i);

           if (i == 0) {
                reset_flag = 1;
                current_batch++;
                printf("Flag reset \n");
           }
           latest_count = i;

     //           printf("Current batch %d\n",current_batch);


           if (current_batch == no_of_batches) {
              exit(1);
              pthread_exit(0);
           }
      }
  }

  close(sock);
  return 0;

}




int main(int argc, char *argv[])
{
  //  pthread_t tcp_thread;
    pthread_t ack_server_thread;
    pthread_t initial_thread;
    int element,ar_set;

    fill_parameters(argv[1],2048,16384);

    server_ip = malloc(20);
    strcpy(server_ip,argv[2]);

    unchanged_batch_size = batch_size;
    char nack_array[batch_size];

    nack_pointer = nack_array;

    host= (struct hostent *) gethostbyname((char *)argv[2]);

    for (ar_set = 0; ar_set < batch_size; ar_set++)
        nack_array[ar_set] = '0';

    printf("%s\n",argv[1]);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
  perror("Socket");
        LOGERR("Client/UDP/Socket\n");
  exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(server_addr.sin_zero),8);
    
    /*    if (pthread_create(&tcp_thread, NULL, tcp_server, NULL)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
    */
    

    mmaped_file  = read_file_to_heap(argv[1]);
    if (mmaped_file == NULL) {
        LOGERR("Client/File Read");
    }    

    if (pthread_create(&initial_thread, NULL, ini_thread,NULL)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
       
    }

    while(start_sending != 1);

    if (pthread_create(&ack_server_thread, NULL, ack_thread, NULL)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    LOGDBG("Sending .....\n");

    while(1) {
     if (current_batch == no_of_batches -1){
        for (element = 0; element < last_batch_size; element++) {
          if (*(nack_pointer+element)=='0') {
                if (current_batch%2 == 0)
                  send_by_seq_no(element);
                else
                  send_by_seq_no(element+unchanged_batch_size);
          }
        }
     }

     else{
        for (element = 0; element < batch_size; element++) {
          if (*(nack_pointer+element)=='0') {
                if (current_batch%2 == 0)
                  send_by_seq_no(element);
                else
                  send_by_seq_no(element+unchanged_batch_size);
          }
        }
     }
    }

    LOGDBG("Sending Complete\n");
    return 0;
}

