#include "macros.h"

// UDP header's structure

struct myheader {
    unsigned short int seq_num;
    unsigned short int dest_port;
    unsigned short int pack_len;
    unsigned short int pack_chksum;
};

struct mytcpheader {
    unsigned int nack_array[MAX_ARQ_SIZE/16];
};
