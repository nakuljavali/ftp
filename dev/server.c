#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"

#define PROTOPORT 7654

int main()
{
  int sock;
  int addr_len, bytes_read;
  char recv_data[1024];
  struct sockaddr_in server_addr , client_addr;


  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    LOGERR("Socket: Creating Socket\n");
    perror("Socket");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PROTOPORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  bzero(&(server_addr.sin_zero),8);

  if (bind(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1) {
      LOGERR("Socket: Binding to server port\n");
      perror("Bind");
      exit(1);
  }

  addr_len = sizeof(struct sockaddr);
  
  LOGDBG("Server listening on 7654\n");
  fflush(stdout);

  FILE *fp;
  fp = fopen("new.txt","a+");
  setbuf(fp, 0);
  while (1)
    {

      bytes_read = recvfrom(sock,recv_data,1024,0,
			    (struct sockaddr *)&client_addr, &addr_len);
        

      recv_data[bytes_read] = '\0';
    
      //      fprintf(fp,"%s", recv_data);
      fwrite(recv_data,1,sizeof(recv_data),fp);
      fflush(stdout);

    }
  return 0;
}
