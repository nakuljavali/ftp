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

#include "macros.h"
#include "customhead.h"
#include "../log.h"

#define ONEGIG 8388608

const char *mmaped_file = NULL;

int sock, bytes_sent = 0;
struct sockaddr_in server_addr;
struct hostent *host;
int start_sending = 0;

char nack_array[MAX_ARQ_SIZE];

void print_array (char arr[MAX_ARQ_SIZE]) 
{
    int i;
    for (i = 0; i < MAX_ARQ_SIZE; i++)
        LOGDBG("%c,",arr[i]);
}

int print_array_count(char arr[MAX_ARQ_SIZE])
{
    int i,count = 0;
    for (i = 0; i < MAX_ARQ_SIZE; i++)
        if (arr[i]=='0') count++;

    LOGDBG("COUNT RECV: %d\n",count);
    fflush(stdout);
    
    return count;
}

void initialize_array (char arr[MAX_ARQ_SIZE])
{
    int i;
    for(i = 0; i < MAX_ARQ_SIZE; i++)
       arr[i] = '0';
}

void send_udp(struct myudpheader* mydata)
{
    bytes_sent = sendto(sock,(void *)mydata, sizeof(struct myudpheader), 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
}


int send_by_seq_no(int seq_no)
{
    struct myudpheader* myudp = (struct myudpheader*) malloc(sizeof(struct myudpheader));
    
    myudp->sequence_no = seq_no;
    memcpy(myudp->payload_data,mmaped_file+seq_no*MAX_DATA_SIZE,MAX_DATA_SIZE);

    send_udp(myudp);
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
  char recv_data[MAX_ARQ_SIZE];

  struct sockaddr_in server_addr,client_addr;
  socklen_t sin_size;

  int i,bytes_recv;

  for(i = 0; i < MAX_ARQ_SIZE; i++)
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

  LOGDBG("I got a connection from (%s , %d)\n",
	 inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

  start_sending = 1;

  while(1) {
      bytes_recv = recv(connected,recv_data,MAX_ARQ_SIZE,MSG_WAITALL);
      if (bytes_recv > 0) {
           LOGDBG("Bytes RECEIVED %d\n",bytes_recv);
           memcpy(nack_array,recv_data,MAX_ARQ_SIZE);
           if (print_array_count(recv_data) == 0)
               pthread_exit(0);
      }
  }

  close(sock);
  return 0;

}

int main(int argc, char *argv[])
{
    pthread_t tcp_thread;
    int element,ar_set;

    host= (struct hostent *) gethostbyname((char *)SERVER_IP);

    for (ar_set = 0; ar_set < MAX_ARQ_SIZE; ar_set++)
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

    LOGDBG("size of udp header %ld\n",sizeof(struct myudpheader));
    LOGDBG("Sending .....\n");

    while(!start_sending);

    while(1) {
        for (element = 0; element < MAX_ARQ_SIZE; element++) {
            if (nack_array[element]=='0')
                send_by_seq_no(element);
        }
    }

    LOGDBG("Sending Complete\n");
    return 0;
}


