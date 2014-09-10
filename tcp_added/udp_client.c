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

#define ONEGIG 8388608

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

void send_udp(struct myudpheader*);
const char *read_file_to_heap(char *);
int send_by_seq_no(int);


const char *mmaped_file = NULL;

int sock, bytes_sent = 0;
struct sockaddr_in server_addr;
struct hostent *host;
int start_sending = 0;

char nack_array[MAX_ARQ_SIZE];

void print_array (char arr[MAX_ARQ_SIZE]) 
{
  int i;
  for(i = 0; i < MAX_ARQ_SIZE; i++)
    printf("%c,",arr[i]);

}


void print_array_count(char arr[MAX_ARQ_SIZE])
{
  int i,count = 0;
  for(i = 0; i < MAX_ARQ_SIZE; i++)
      if (arr[i]=='0')
            count++;

  printf("COUNT RECV: %d\n",count);
  fflush(stdout);
}

void initialize_array (char arr[MAX_ARQ_SIZE])
{
  int i;
  for(i = 0; i < MAX_ARQ_SIZE; i++)
     arr[i] = '0';
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
    exit(1);
  }

  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
    perror("Setsockopt");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(TCP_PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  bzero(&(server_addr.sin_zero),8);

  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))
      == -1) {
    perror("Unable to bind");
    exit(1);
  }

  if (listen(sock, 5) == -1) {
    perror("Listen");
    exit(1);
  }
  sin_size = sizeof(struct sockaddr_in);

  connected = accept(sock, (struct sockaddr *)&client_addr,&sin_size);

  printf("\n I got a connection from (%s , %d)",
	 inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

  start_sending = 1;

  int total_received = 0;

  while(1) {

    bytes_recv = recv(connected,recv_data,MAX_ARQ_SIZE,MSG_WAITALL);
    printf("Bytes RECEIVED %d\n",bytes_recv);


           if (bytes_recv > 0) {
              memcpy(nack_array,recv_data,MAX_ARQ_SIZE);
              printf("bytes received: %d\n",bytes_recv);
              print_array_count(recv_data);
           }

  }



  /*  while (1)
    {
      bytes_recv = 0;
      total_received = 0;
      while( total_received < 32768 ) {
          bytes_recv = recv(connected,recv_data+total_received,MAX_ARQ_SIZE,0);

          total_received += bytes_recv;

           if (bytes_recv > 0) {
              memcpy(nack_array,recv_data,MAX_ARQ_SIZE);
              printf("bytes received: %d\n",bytes_recv);
              print_array_count(recv_data);
           }

      }
      printf("TOTOAL RECEIVED %d\n",total_received);

    }
  */

  close(sock);
  return 0;

}





int main(void)
{
 
    pthread_t tcp_thread;

   // set socket parameters
    host= (struct hostent *) gethostbyname((char *)SERVER_IP);

    int j;
    for(j = 0; j < MAX_ARQ_SIZE; j++)
       nack_array[j] = '0';


    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
	perror("socket");
	exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(server_addr.sin_zero),8);

    
    if(pthread_create(&tcp_thread, NULL, tcp_server, NULL)) {

      fprintf(stderr, "Error creating thread\n");
      return 1;

      }

    //mmaped_file  = read_file_to_heap("/mnt/onegig.bin");
    mmaped_file  = read_file_to_heap("/mnt/onegig_nodeB.bin");
    
    assert( mmaped_file != NULL);
 //send_udp(12);
	printf("size of udp header %ld\n",sizeof(struct myudpheader));
	printf("Sending .....");
    int element;

    //    start_sending = 1;
    while(!start_sending);

    while(1) {
        for (element = 0; element < MAX_ARQ_SIZE; element++) {
            if (nack_array[element]=='0')
                send_by_seq_no(element);

        }
    }


    printf("\n Done....");
    return 0;

}





// function definition follow here 


int send_by_seq_no(int seq_no)
{
    struct myudpheader* myudp = (struct myudpheader*) malloc(sizeof(struct myudpheader));
    
    myudp->sequence_no = seq_no;
    memcpy(myudp->payload_data,mmaped_file+seq_no*MAX_DATA_SIZE,MAX_DATA_SIZE);
    //    printf("Seq no %d\n",myudp->sequence_no);
    //printf("Data = %s",myudp->payload_data);
    //printf("\nMemcopied the following to data.....\n");
    //fwrite(myudp, 1 ,10, stdout );
   // printf("\n");
  

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
  assert(fd != -1);
  fstat(fd, &sb);
  printf("Size: %lu\n", (uint64_t)sb.st_size);
  memblock = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
 
  if (memblock == MAP_FAILED) handle_error("mmap");
  printf("\nMmap SUCCESSFULL.........\n");
  close(fd);
  return memblock;
/*
  printf("after mmap\n");
  FILE *fp;

  printf("fwrite\n");
  fp = fopen( "output.bin" , "w" );
  assert(fp != NULL);
  fwrite(memblock, 1 , (uint64_t)sb.st_size, fp );
  return 0;
*/
}







void send_udp(struct myudpheader* mydata)
{

bytes_sent = sendto(sock,(void *)mydata, sizeof(struct myudpheader), 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
//printf("\n Bytes send = %d",bytes_sent);

}

