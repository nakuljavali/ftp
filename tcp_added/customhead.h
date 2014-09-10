#include "macros.h"
// UDP header's structure

struct myudpheader {
    unsigned short int sequence_no;
    char payload_data[MAX_DATA_SIZE];
};

struct mytcpheader {
    char nack_array[MAX_ARQ_SIZE];
};
