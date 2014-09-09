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

#include "macros.h"

#include "customhead.h"

int main()
{
        int sock;
        socklen_t addr_len;
	int  bytes_read;
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
	while (1)
	{

          bytes_read = recvfrom(sock,recv_data,sizeof(struct myudpheader),0,(struct sockaddr *)&client_addr, &addr_len);
	  

          struct myudpheader* recv_payload = (struct myudpheader*)(recv_data);

//          printf("\n seq_no received =%d", recv_payload->sequence_no);
//	  fflush(stdout);          
//printf("\n data =");
	memcpy(heap_mem+(recv_payload->sequence_no)*MAX_DATA_SIZE,recv_payload->payload_data,MAX_DATA_SIZE);
	if(recv_payload->sequence_no == MAX_DATA_SIZE-1)
	 {
	  printf("\n Writing to file\n");
	  fwrite(heap_mem,MAX_DATA_SIZE,MAX_DATA_SIZE,fp);
	  fclose(fp);
	  exit(0);
	}
//	  recv_data[bytes_read] = '\0';

//          printf("\n(%s , %d) said : ",inet_ntoa(client_addr.sin_addr),
  //                                     ntohs(client_addr.sin_port));

        }
        return 0;
}
