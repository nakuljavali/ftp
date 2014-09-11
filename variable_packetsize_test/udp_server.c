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
#include <sys/time.h>

#include "macros.h"
#include "customhead.h"

struct timeval  tv1, tv2;


int main()
{
        int sock;
        socklen_t addr_len;
        char recv_data[sizeof(struct myudpheader)];
        struct sockaddr_in server_addr , client_addr;


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
	int payload_data_size = sizeof(struct myudpheader) - 2;
	int total_data = sizeof(struct myudpheader);
	int starttime = 1;
	while (1)
	{

          recvfrom(sock,recv_data,sizeof(struct myudpheader),0,(struct sockaddr *)&client_addr, &addr_len);
	 
	  if(starttime)
	  {
		gettimeofday(&tv1, NULL);
		starttime = 0;
	  }



          struct myudpheader* recv_payload = (struct myudpheader*)(recv_data);

          printf("\n seq_no received =%d", recv_payload->sequence_no);
	  if(recv_payload->sequence_no < NO_OF_PACKETS){
	  	memcpy(heap_mem+(recv_payload->sequence_no)*payload_data_size,recv_payload->payload_data,payload_data_size);
	  }

	  if(recv_payload->sequence_no == NO_OF_PACKETS)
	  {
	    memcpy(heap_mem+(recv_payload->sequence_no)*payload_data_size,recv_payload->payload_data,LAST_PACKET);
	   printf("\n Last packet written of size = %d .........",LAST_PACKET); 
	    printf("\n Writing to file\n");
	    fwrite(heap_mem,1,MAX_DATA_SIZE*MAX_DATA_SIZE,fp);
	    gettimeofday(&tv2, NULL);
	    fclose(fp);
	    break;
	  }

        }
	printf ("Total time = %f seconds\n",
         (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
         (double) (tv2.tv_sec - tv1.tv_sec));
        return 0;
}
