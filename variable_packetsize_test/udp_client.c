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

#include "macros.h"
#include "customhead.h"


#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

void send_udp(void*,int);
const char *read_file_to_heap(char *);
int send_by_seq_no(int);


const char *mmaped_file = NULL;

int sock, bytes_sent = 0;
struct sockaddr_in server_addr;
struct hostent *host;



int payload_data_size = 0;

int main(void)
{
    // set socket parameters
    host= (struct hostent *) gethostbyname((char *)SERVER_IP);


    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
	perror("socket");
	exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(server_addr.sin_zero),8);




    //mmaped_file  = read_file_to_heap("/mnt/onegig.bin");
    mmaped_file  = read_file_to_heap("/mnt/onegig_nodeB.bin");
    
    assert( mmaped_file != NULL);   
 //send_udp(12);
	printf("Sending .....\n");
    int i=0;
    for(i=0;i<=NO_OF_PACKETS;i++)
	{
          send_by_seq_no(i);
	}
	printf("Done....\n");
    return 0;

}





// function definition follow here 


int send_by_seq_no(int seq_no)
{
//    printf("\n the data size = %ld",sizeof(struct myudpheader->payload_data));
    if(seq_no == NO_OF_PACKETS)
    {

    struct mylastpacket* myudp = (struct mylastpacket*) malloc(sizeof(struct mylastpacket));
    payload_data_size = sizeof(myudp->payload_data);
    myudp->sequence_no = seq_no;
    memcpy(myudp->payload_data,mmaped_file+seq_no*LARGE_DATA,payload_data_size);
    printf("Seq no %d\n",myudp->sequence_no);
    send_udp((void *)myudp,seq_no);
    free(myudp); 
    }
    else
    {
    struct myudpheader* myudp = (struct myudpheader*) malloc(sizeof(struct myudpheader));
    payload_data_size=sizeof(myudp->payload_data);
    myudp->sequence_no = seq_no;
    printf("Seq no %d\n",myudp->sequence_no);
    memcpy(myudp->payload_data,mmaped_file+seq_no*payload_data_size,payload_data_size);
    send_udp((void *)myudp,seq_no);
    free(myudp); 
    }
    //printf("Data = %s",myudp->payload_data);
    //printf("\nMemcopied the following to data.....\n");
    //fwrite(myudp, 1 ,10, stdout );
   // printf("\n");
  

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
}







void send_udp(void *mydata,int seq_no)
{
  if(seq_no < NO_OF_PACKETS)
  {
  	bytes_sent = sendto(sock,(void *)mydata, sizeof(struct myudpheader), 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
  	printf("Bytes sent = %ld\n",sizeof(struct myudpheader));
  }
  else
  {
	bytes_sent = sendto(sock,(void *)mydata, sizeof(struct mylastpacket), 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
  	printf("Bytes sent = %ld\n",sizeof(struct mylastpacket));
  }
}

