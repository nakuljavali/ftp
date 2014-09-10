#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[])
{
  const char *memblock;
  int fd;
  struct stat sb;

  fd = open(argv[1], O_RDONLY);
  fstat(fd, &sb);
  printf("Size: %lu\n", (uint64_t)sb.st_size);
  memblock = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (memblock == MAP_FAILED) handle_error("mmap");

  printf("after mmap\n");
  FILE *fp;

  printf("fwrite\n");
  fp = fopen( "file.txt" , "w" );
  fwrite(memblock+10, 1 , (uint64_t)sb.st_size-10 , fp );
  return 0;
}
