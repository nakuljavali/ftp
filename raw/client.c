#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
 
struct pseudo_header
{
  u_int32_t source_address;
  u_int32_t dest_address;
  u_int8_t placeholder;
  u_int8_t protocol;
  u_int16_t udp_length;
};
 
unsigned short csum(unsigned short *ptr,int nbytes) 
{
  register long sum;
  unsigned short oddbyte;
  register short answer;
 
  sum=0;
  while(nbytes>1) {
    sum+=*ptr++;
    nbytes-=2;
  }
  if(nbytes==1) {
    oddbyte=0;
    *((u_char*)&oddbyte)=*(u_char*)ptr;
    sum+=oddbyte;
  }
 
  sum = (sum>>16)+(sum & 0xffff);
  sum = sum + (sum>>16);
  answer=(short)~sum;
     
  return(answer);
}
 
int main (void)
{

  int s = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);
     
  if(s == -1)
    {
      perror("Failed to create raw socket");
      exit(1);
    }
     
  char datagram[4096] , source_ip[32] , *data , *pseudogram;
     
  memset (datagram, 0, 4096);
     
  struct iphdr *iph = (struct iphdr *) datagram;
     
  struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct ip));
     
  struct sockaddr_in sin;
  struct pseudo_header psh;

  data = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);
  strcpy(data , "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
     
  strcpy(source_ip , "192.168.1.2");
     
  sin.sin_family = AF_INET;
  sin.sin_port = htons(80);
  sin.sin_addr.s_addr = inet_addr ("10.1.1.2");
     
  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 0;
  iph->tot_len = sizeof (struct iphdr) + sizeof (struct udphdr) + strlen(data);
  iph->id = htonl (54321);
  iph->frag_off = 0;
  iph->ttl = 255;
  iph->protocol = IPPROTO_UDP;
  iph->check = 0;
  iph->saddr = inet_addr ( source_ip );
  iph->daddr = sin.sin_addr.s_addr;
     
  iph->check = csum ((unsigned short *) datagram, iph->tot_len);

  udph->source = htons (6666);
  udph->dest = htons (8622);
  udph->len = htons(8 + strlen(data));
  udph->check = 0;
     
  psh.source_address = inet_addr( source_ip );
  psh.dest_address = sin.sin_addr.s_addr;
  psh.placeholder = 0;
  psh.protocol = IPPROTO_UDP;
  psh.udp_length = htons(sizeof(struct udphdr) + strlen(data) );
     
  int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
  pseudogram = malloc(psize);
     
  memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
  memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));
     
  udph->check = csum( (unsigned short*) pseudogram , psize);
     
  int count = 10;
  while (count > 0)
  {
    if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
      {
	perror("sendto failed");
      }
    else
      {
	printf ("Packet Send. Length : %d \n" , iph->tot_len);
      }
  
    count--;
  }
     
  return 0;
}
