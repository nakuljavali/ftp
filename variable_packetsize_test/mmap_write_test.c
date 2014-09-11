#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "macros.h"
#include "customhead.h"

#define FILE_LENGTH 8388608
#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)






/* Return a uniformly random number in the range [low,high].  */

int random_range (unsigned const low, unsigned const high)
{
  unsigned const range = high - low + 1;
  return low + (int) (((double) range) * rand () / (RAND_MAX + 1.0));
}

const char *read_file_to_heap(char *file_name)
{

  const char *memblock;
  int fd;
  struct stat sb;

  fd = open(file_name, O_RDONLY);
  assert(fd != -1);
  fstat(fd, &sb);
  printf("Size: %lu\n", (uint64_t)sb.st_size);
  memblock = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);

  if (memblock == MAP_FAILED) handle_error("mmap");
  printf("\nMmap SUCCESSFULL.........\n");
  close(fd);
  return memblock;
}

int main (int argc, char* const argv[])
{
//  int fd;
  void* file_memory;

  /* Seed the random number generator.  */
  //srand (time (NULL));
  const char* heap_memory = read_file_to_heap("/mnt/onegig_nodeB.bin");
  /* Prepare a file large enough to hold an unsigned integer.  */
// fd = open (argv[1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
FILE* fd =fopen("myoutput.bin","w");
fwrite(heap_memory,1,FILE_LENGTH,fd);
fclose(fd);
/*  lseek (fd, FILE_LENGTH+1, SEEK_SET);
  write (fd, "", 1);
  lseek (fd, 0, SEEK_SET);
fwrite(str , 1 , sizeof(str) , fp );*/

  /* Create the memory-mapping.  */
 /* file_memory = mmap (0, FILE_LENGTH, PROT_WRITE, MAP_SHARED, fd, 0);
  if (file_memory == MAP_FAILED) handle_error("mmap");

  memcpy(file_memory,heap_memory,FILE_LENGTH);
  close (fd);*/
  /* Write a random integer to memory-mapped area.  */
//  sprintf((char*) file_memory, "%d\n", random_range (-100, 100));
  /* Release the memory (unnecessary since the program exits).  */
//  munmap (file_memory, FILE_LENGTH);

  return 0;
}
