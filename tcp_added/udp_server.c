/* udp_server.c */

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
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>

#include <pcap.h>

#include "../macros.h"
#include "../customhead.h"
#include "../log.h"

#include "../myparameters.c"

struct timeval  tv1, tv2,tv3;

pthread_t tcp_client,recv_thread,recv_thread_pcap;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int stop_flag = 0;
int current_batch = 0;
char *nack_pointer = NULL;
int batch_set_flag = 0;
int create_array_flag =0;
int total_packets_received = 0;

int packet_counter = 0;

//############################################
//##########Receiver Thread Parameters########
    FILE *fp = NULL;
    char *heap_mem = NULL;
    int sequence_no = 0;
//############################################



int print_array_count(char arr[batch_size]){
    int i,count = 0;
    for(i = 0; i < batch_size; i++)
        if (arr[i]=='0')
            count++;

    LOGDBG("COUNT: %d\n",count);
    printf("current batch: %d\n",current_batch);
    fflush(stdout);
    
    return count;
}

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
  char* recv_data = (char *)(packet + 14 + 20 + 8);
  packet_counter++;  

       memcpy(&sequence_no,recv_data,2);

       printf("seq no %d\n",sequence_no);

        if (current_batch == no_of_batches - 1) {
            batch_size = last_batch_size;

            if(current_batch%2 == 0){

              if(sequence_no == last_batch_size - 1)

                packet_size = last_packet_size;

            }

            else if(current_batch%2 ==1){

              if((sequence_no-batch_size) == last_batch_size - 1)

                packet_size = last_packet_size;

            }

        }
        if(current_batch%2 == 0){

            if(0 <= sequence_no && sequence_no <= batch_size -1){

                memcpy(heap_mem+(current_batch*batch_size*packet_size)+(packet_size*sequence_no),recv_data+2,packet_size);

                *(nack_pointer+sequence_no) = '1';

            }

            total_packets_received++;

        }
        else if(current_batch%2 == 1){

          printf("Seq no: %d\n",sequence_no);

            if(batch_size <= sequence_no &&  sequence_no <= 2*batch_size - 1){

              printf("Here\n");

              memcpy(heap_mem+(current_batch*batch_size*packet_size)+(packet_size*(sequence_no - batch_size)),recv_data+2,packet_size);

                *(nack_pointer+(sequence_no-batch_size)) = '1';

            }

            total_packets_received++;

        }



    

        if(stop_flag){

            printf("Writing to file\n");

            fwrite(heap_mem,1,filesize,fp);

            fclose(fp);

            exit(0);

        }


}

void *receiver_pcap(void *args)
{

  // intialize udp parameters 

  

  while(!batch_set_flag); // wait until the  parameters are received from the client
  heap_mem = (char *)malloc((filesize));

  char *dev = "eth0";
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *handle;

  char filter_exp[] = "udp port 7654";
  struct bpf_program fp;/* compiled filter program (expression) */

  handle = pcap_open_live(dev, (packet_size+2), 1, 1000, errbuf);
  if (handle == NULL) {
  fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
  exit(EXIT_FAILURE);
  }

  /* compile the filter expression */
  if (pcap_compile(handle, &fp, filter_exp, 0, 0) == -1) {
    fprintf(stderr, "Couldn't parse filter %s: %s\n",
	    filter_exp, pcap_geterr(handle));
    exit(EXIT_FAILURE);
  }

  /* apply the compiled filter */
  if (pcap_setfilter(handle, &fp) == -1) {
    fprintf(stderr, "Couldn't install filter %s: %s\n",
	    filter_exp, pcap_geterr(handle));
    exit(EXIT_FAILURE);
  }

  /* now we can set our callback function */
  pcap_loop(handle, -1 , got_packet, NULL);

  return NULL;
}

void *tcp_thread(void *args){

    int sockfd, n, i, count;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct infoheader info;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOGERR("ERROR creating TCP socket\n");
        exit(1);
    }

    server = gethostbyname(CLIENT_IP);
    if (server == NULL) {
        LOGERR("ERROR no such host\n");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(TCP_PORT);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        LOGERR("ERROR connecting to TCP server\n");
        exit(1);
    }

    printf("TCP client connected\n");

    // TCP reading the first info header

    
    n = read(sockfd,(void*)&info, sizeof(info));
    if (n < 0) 
        LOGERR("ERROR reading from socket\n");



    packet_size = info.size_pkt;
    filesize = info.size_file;
    no_of_packets = info.no_packets;
    batch_size = info.size_batch;
    no_of_batches = info.no_batches;
    last_batch_size = info.size_last_batch;
    last_packet_size = info.size_last_packet;
    batch_set_flag = 1;

    LOGDBG("packtet size : %d\n", packet_size);
    LOGDBG("file size : %d\n", filesize );
    LOGDBG("batch size : %d\n", batch_size);
    LOGDBG("no_batches : %d\n", no_of_batches);
    LOGDBG("no_packets : %d\n", no_of_packets);
    LOGDBG("size_last_batch : %d\n", last_batch_size);
    LOGDBG("size_last_packet : %d\n", last_packet_size);

    LOGDBG("Received the info header\n");

    while(!create_array_flag);
    char nack_array[batch_size];
    char arr[batch_size];
    nack_pointer = nack_array;

    for (i = 0; i < batch_size; i++) {
        arr[i] = '0';
        nack_array[i]= '0';
    }


    print_array_count(nack_pointer);


    while(1){

        pthread_mutex_lock(&lock);
        memcpy(&arr,nack_pointer,batch_size);
        pthread_mutex_unlock(&lock);
        print_array_count(nack_pointer);


        count = 0;
        for(i =0; i< batch_size; i++){
            if (arr[i]=='0')
                count++;
        }

        if (count < 2000)
           usleep (2000*1500); 
        else
           usleep(count*PACKET_TIME);

        pthread_mutex_lock(&lock);
        memcpy(&arr,nack_pointer,batch_size);
        pthread_mutex_unlock(&lock);

        LOGDBG("After sending");
        printf("packet received: %d\n",packet_counter);      
        count = 0;
        for(i =0; i< batch_size; i++){
            if (arr[i]=='0')
                count++;
        }
        print_array_count(arr);

        n = send(sockfd, (void*)&arr, sizeof(arr), 0);

        if (n < 0) {
            LOGERR("ERROR writing to socket\n");
            exit(1);
        }

        if (0 == count){

           // n = send(sockfd, (void*)&arr, sizeof(arr), 0);
            current_batch++;
            if(current_batch == no_of_batches){
                stop_flag = 1;
                close(sockfd);
                pthread_exit(0);
            }

            if(current_batch == no_of_batches - 1) batch_size = last_batch_size;
                for (i = 0; i < batch_size; i++) {
                    nack_array[i] = '0';
                        arr[i] = '0';

	    }

		usleep(100);

        }

    }

}

int main(){
    //int sock
    int i;
    //socklen_t addr_len;
    int bytes_read = 0;


    fp = fopen("/mnt/output_nodeb.bin","w");
    assert(fp != NULL);
    //struct sockaddr_in server_addr , client_addr;
    bytes_read = bytes_read;


    if(pthread_create(&tcp_client, NULL, tcp_thread, "tcp_client") != 0){
        LOGERR("ERROR creating TCP thread\n");
        exit(1);
    }

    while(!batch_set_flag);

    char nack_array[batch_size];
    nack_pointer = nack_array;


    for (i = 0; i < batch_size; i++) {
        nack_array[i] = '0';
    }
    create_array_flag =1;

    if(pthread_create(&recv_thread_pcap, NULL, receiver_pcap,"udp receiver thread") != 0){
        LOGERR("ERROR creating pcap receiver thread");
        exit(1);
    }
    pthread_join(recv_thread_pcap,NULL);
    return 0;
}
