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
/*
int packet_size = 32768;
int filesize = 1073741824;
int no_of_packets = 32768;
int batch_size = 256;
int no_of_batches = 128;
int last_batch_size = 0;
int last_packet_size = 0;
*/

void  fill_parameters(char *input_file,int pkt_size,int bch_size)
{
	int file=0, no_of_packets = 0;
     	if((file=open(input_file,O_RDONLY)) < -1)
            exit(1);
 
	struct stat fileStat;
    	if(fstat(file,&fileStat) < 0)    
            exit(1);


	filesize = fileStat.st_size;
	packet_size = pkt_size;
	batch_size = bch_size;
        no_of_packets = filesize / packet_size;
	no_of_batches = no_of_packets / batch_size;
	last_batch_size = no_of_packets -  no_of_batches * batch_size;
	last_packet_size = filesize - no_of_packets * packet_size;
	printf("Information for %s\n",input_file);
   	printf("---------------------------\n");
    	printf("File Size:         %ld bytes\n",fileStat.st_size);
    	printf("Packet Size:       %d bytes\n",packet_size);
    	printf("No of Packets:     %d bytes\n",no_of_packets);
    	printf("Batch size:        %d bytes\n",batch_size);
    	printf("No of batches:     %d bytes\n",no_of_batches);
    	printf("Last Packet Size:  %d bytes\n",last_packet_size);
    	printf("Last Batch Size:   %d bytes\n",last_batch_size);
	
}
