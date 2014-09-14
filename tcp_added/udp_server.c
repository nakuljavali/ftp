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



pthread_t tcp_thread_t;
pthread_t ack_thread_t;
pthread_t write_thread_t;

int stop_flag = 0;
int current_batch = 0;
char *nack_pointer = NULL;
int batch_set_flag = 0;
int create_array_flag =0;

char *heap_mem=NULL;

//ACK thread parameters
int acksock, true;
socklen_t addr_len;
int bytes_read = 0;
struct hostent *host;

struct sockaddr_in ack_server;

char* client_ip = NULL;
// dont change this batch size 
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

void send_array_with_batch()
{
  int bytes_sent;

  char* arrbat = (char *) malloc(batch_size+4);

  memcpy(arrbat,&current_batch,4);
  memcpy(arrbat+4, nack_pointer, batch_size);
    
  bytes_sent = sendto(acksock,(void *)arrbat, (batch_size+4) , 0,(struct sockaddr *)&ack_server, sizeof(struct sockaddr));
	
  bytes_sent = bytes_sent;
  usleep(10000);    
  free(arrbat);
}

void *tcp_thread(void *args){

  int sockfd, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  struct infoheader info;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    LOGERR("ERROR creating TCP socket\n");
    exit(1);
  }

  server = gethostbyname(client_ip);
  if (server == NULL) {
    LOGERR("ERROR no such host\n");
    exit(1);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(TCP_PORT);

  while(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
    LOGERR("ERROR connecting to TCP server........Trying again\n");
    //exit(1);
  }

  printf("TCP client connected\n");

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

  pthread_exit(0);

}


void *ack_thread(void *args){


  host= (struct hostent *) gethostbyname(client_ip);

  if((acksock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    LOGERR("ERROR creating UDP socket\n");
    exit(1);
  }

  ack_server.sin_family = AF_INET;
  ack_server.sin_port = htons(ACK_PORT);
  ack_server.sin_addr = *((struct in_addr *)host->h_addr);
  bzero(&(ack_server.sin_zero),8);

  if (setsockopt(acksock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
    LOGERR("Client/TCP/Socket/Reuseaddr");
    exit(1);
  }

  LOGDBG("ACK Client starts");

  while(!create_array_flag);


  while (1) {


    int count,i;
    count =  print_array_count(nack_pointer);

    if(0 == count){
      for(i = 0; i < 100; i++){
	send_array_with_batch();
      }
      current_batch++;
      memset(nack_pointer, '0', batch_size);

      //      printf("Current Batch %d\n",current_batch);
    }
    else
      send_array_with_batch();

    if(current_batch == no_of_batches)
      {

        pthread_exit(0);

	//	printf("STOP FLAG IS 1!!!!!!!!\n");
	/*	FILE *fp = fopen("/tmp/output.bin","w");
	assert(fp != NULL);
	assert(heap_mem!= NULL);
	printf("Writing to file\n");
	fwrite(heap_mem,1,filesize,fp);
	fclose(fp);
	exit(0); */
      }

  }

}


void *write_thread(void *args) 
{
  FILE *fp_batch = fopen("/tmp/batch.bin","w");
  assert(fp_batch != NULL);
  assert(heap_mem != NULL);

  int written_batch = 0;
  int writing_size = batch_size*packet_size;

  while (1) {

    if (current_batch == written_batch+1) {

      printf("Writing batch %d\n",written_batch);

        if(current_batch == no_of_batches) {
           writing_size = last_batch_size*packet_size - (packet_size - last_packet_size);
           fwrite(heap_mem+written_batch*batch_size*packet_size,1,writing_size,fp_batch);
           fclose(fp_batch);
           exit(1);
          }

        fwrite(heap_mem+written_batch*batch_size*packet_size,1,writing_size,fp_batch);

        written_batch++;

      }
  }
  return 0;

}


int main(int argc, char *argv[]){
  int sock;
  socklen_t addr_len;
  int bytes_read = 0;

  struct sockaddr_in server_addr , client_addr;
  bytes_read = bytes_read;

  client_ip = malloc(20);
  strcpy(client_ip,argv[1]);

  /*
  if(pthread_create(&tcp_thread_t, NULL, tcp_thread, "tcp_thread") != 0){
    LOGERR("ERROR creating TCP thread\n");
    exit(1);
    }*/

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

  struct infoheader *info = malloc(sizeof(struct infoheader));

  while (1) {

    int bytes_recv,data_set = 0;
  
    bytes_recv = recvfrom(sock,(void*)info,sizeof(struct infoheader),0,(struct sockaddr *)&client_addr,&addr_len);

    if (info->sync1 == 65536 && info->sync2 == 65536) {
      if( data_set == 0) {
         packet_size = info->size_pkt;
         filesize = info->size_file;
         batch_size = info->size_batch;
         no_of_batches = info->no_batches;
         no_of_packets = info->no_packets;
         last_batch_size = info->size_last_batch;
         last_packet_size = info->size_last_packet;
         batch_set_flag  = 1;
         data_set = 1;
      }
    } else {
      break;
    }

  }

  if(pthread_create(&ack_thread_t, NULL, ack_thread, "ack_thread") != 0){
    LOGERR("ERROR creating ACK thread\n");
    exit(1);
  }

  if(pthread_create(&write_thread_t, NULL, write_thread, "write_thread") != 0){
    LOGERR("ERROR creating ACK thread\n");
    exit(1);
  }

  while(!batch_set_flag);

  unchanged_batch_size = batch_size;
  heap_mem = (char *)malloc((no_of_packets * packet_size));


  char nack_array[batch_size];
  nack_pointer = nack_array;

  memset(nack_array,'0',batch_size);

  create_array_flag =1;



  char recv_data[packet_size+2];
  int sequence_no = 0;

  printf("Entering Receiving loop\n");

  while (1){


    bytes_read = recvfrom(sock,recv_data,packet_size+2, 0,(struct sockaddr *)&client_addr, &addr_len);

    memcpy(&sequence_no,recv_data,2);
    printf("SERVER : received seq no : %d, current batch: %d, total batches: %d\n",sequence_no,current_batch,no_of_batches);

    
if (current_batch != no_of_batches - 1) 
{
    if(current_batch%2 == 0)
    {
      if((0 <= sequence_no && sequence_no <= unchanged_batch_size -1) &&  (*(nack_pointer+sequence_no) != '1'))
      {
	memcpy(heap_mem+(current_batch*unchanged_batch_size*packet_size)+(packet_size*sequence_no),recv_data+2,packet_size);
	*(nack_pointer+sequence_no) = '1';
      }
    }
    else if(current_batch%2 == 1)
    {
      if((unchanged_batch_size <= sequence_no &&  sequence_no <= 2*unchanged_batch_size - 1) && (*(nack_pointer+(sequence_no-unchanged_batch_size)) != '1') )
      {
	memcpy(heap_mem+(current_batch*unchanged_batch_size*packet_size)+(packet_size*(sequence_no - unchanged_batch_size)),recv_data+2,packet_size);
	*(nack_pointer+(sequence_no-unchanged_batch_size)) = '1';

      }
    }
}
else
{
  if(current_batch%2 == 0)
    {
      if((0 <= sequence_no && sequence_no <= last_batch_size -1) &&  (*(nack_pointer+sequence_no) != '1'))
      {
        memcpy(heap_mem+(current_batch*unchanged_batch_size*packet_size)+(packet_size*sequence_no),recv_data+2,packet_size);
        *(nack_pointer+sequence_no) = '1';
      }
    }
    else if(current_batch%2 == 1)
    {
      if(unchanged_batch_size <= sequence_no &&  sequence_no <= (unchanged_batch_size +last_batch_size - 1)&&(*(nack_pointer+(sequence_no-unchanged_batch_size)) != '1'))
      {
        memcpy(heap_mem+(current_batch*unchanged_batch_size*packet_size)+(packet_size*(sequence_no - unchanged_batch_size)),recv_data+2,packet_size);
        *(nack_pointer+(sequence_no-unchanged_batch_size)) = '1';
      }
    }

}
  }
  return 0;
}
