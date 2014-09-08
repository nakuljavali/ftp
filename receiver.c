#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

FILE *fp;
int flag;

int copying(char *buffer, int sequence_no){

	char *buffer_cut = (char*)(malloc((sizeof(char))*30));
	// Creating 10Mb file
	while (flag == 1){
		fp = fopen("output.txt", "w");
		fseek(fp, 1024*1024, SEEK_SET);
		fputc('\n', fp);
		flag =0;
	}

	fseek(fp, sequence_no * 8, SEEK_SET);
	fwrite(buffer, 2, sizeof(buffer), fp);

return 0;
}

int main(){

	flag = 1;


	fclose(fp);

	return 0;
}




