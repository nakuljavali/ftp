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

int sock, bytes_sent = 0;
struct sockaddr_in server_addr;
struct hostent *host;
int start_sending = 0;
int current_batch = 0;
int reset_flag = 0;
int total_packets_sent = 0;

char *nack_pointer = NULL;

int print_array_count(char arr[batch_size])
{
    int i,count = 0;
    for (i = 0; i < batch_size; i++)
        if (arr[i]=='0') count++;

    //    LOGDBG("COUNT RECV: %d",count);
    fflush(stdout);
    
    return count;
}

void send_udp(void* mydata)
{
    bytes_sent = sendto(sock,(void *)mydata, (packet_size+2) , 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
}


int send_by_seq_no(int seq_no)
{
    if(current_batch%2 ==0)
      {if ( (current_batch == no_of_batches-1) && (seq_no == last_batch_size-1) ) packet_size=last_packet_size;}
    else 
      {if ( (current_batch == no_of_batches-1) && (seq_no-batch_size == last_batch_size-1) ) packet_size=last_packet_size; }
    char* myudp = (char *) malloc(packet_size+2);
   
    memcpy(myudp,&seq_no,2);

    if(current_batch%2 == 0)
      memcpy(myudp+2,mmaped_file+(current_batch*batch_size*packet_size)+(seq_no*packet_size),packet_size);
    else if(current_batch%2 == 1)
      memcpy(myudp+2,mmaped_file+(current_batch*batch_size*packet_size)+((seq_no-batch_size)*packet_size),packet_size);
    send_udp((void*)myudp);
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
  
  start_sending = 1;

  LOGDBG("Listening on TCP Port, to receive nack data");

  while(1) {
      bytes_recv = recv(connected,recv_data,batch_size,MSG_WAITALL);
      if (bytes_recv > 0) {
	//           LOGDBG("Bytes RECEIVED %d",bytes_recv);
           memcpy(nack_pointer,recv_data,batch_size);

           int i;
           i = print_array_count(recv_data);
           printf("ARRAY RECEIVED OF COUNT %d\n",i);
           if (i == 0)reset_flag = 1; 

           if (print_array_count(recv_data) == 0) current_batch++;
           printf("Current batch: %d\n",current_batch);

           printf("Total packet count: %d\n",total_packets_sent);
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
    pthread_t tcp_thread;
    int element,ar_set;

    fill_parameters(argv[1],32768,8192);

    char nack_array[batch_size];

    nack_pointer = nack_array;

    host= (struct hostent *) gethostbyname((char *)SERVER_IP);

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
    
    if (pthread_create(&tcp_thread, NULL, tcp_server, NULL)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    mmaped_file  = read_file_to_heap(argv[1]);
    if (mmaped_file == NULL) {
        LOGERR("Client/File Read\n");
    }    

    LOGDBG("Sending .....\n");

    while(!start_sending);
    
    while(1) {
        if (current_batch == no_of_batches -1) batch_size = last_batch_size;
        for (element = 0; element < batch_size; element++) {
            if (reset_flag == 1) {
	      element = 0;
              reset_flag = 0;
              printf("resetting flag\n");
              printf("new batch: %d\n",current_batch);
            }
	    //         	LOGDBG("Current Batch %d, Current Seq %d",current_batch,element);
  	    if (*(nack_pointer+element)=='0') {
                if (current_batch%2 == 0)
                    send_by_seq_no(element);
                else
                    send_by_seq_no(element+batch_size);

		total_packets_sent++;

            }
        }
    }

    LOGDBG("Sending Complete\n");
    return 0;
}


