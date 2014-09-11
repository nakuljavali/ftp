#include "macros.h"


int packet_size = 32768;
int filesize = 1073741824;
int no_of_packets = 32768;
int batch_size = 256;
int no_of_batches = 128;
int last_batch_size = 0;
int last_packet_size = 0;


// UDP header's structure
void  fill_parameters(char *,int);


struct myudpheader {
    unsigned short int sequence_no;
    char payload_data[LARGE_DATA];
};

struct mytcpheader {
    unsigned int nack_array[MAX_ARQ_SIZE/16];
};

struct mylastpacket{
    unsigned short int sequence_no;
    char payload_data[LAST_PACKET];
};
