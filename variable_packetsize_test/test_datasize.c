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

struct teststruct{
	char data[8];
};

int payload_data_size = sizeof(struct mylargeheader) - 2;
int main()
{	
	char buffer[] = "niranjan12345678";
	void* arr = (void *) malloc(16);
	memcpy(arr,buffer,16);

	struct teststruct* myarr = (struct teststruct*)arr;
	fwrite(myarr->data,1,8,stdout);
	printf("\n value %d\n",ONEGIG/payload_data_size);

	return 0;
}
