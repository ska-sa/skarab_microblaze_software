#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysexits.h>

#include <stdio.h>
#include <stdint.h>

#define MODULO16 16321  /* largest 16-bit prime value */

void usage(char *name)
{
  printf("usage: %s -hf file\n", name);
  printf("-f file  BIN file to process\n");
  printf("-v       verbose\n");
  printf("-h       this help\n");
  printf("\n");
}

int main(int argc, char **argv){

  uint16_t a = 1;
  uint16_t b = 0;
  uint32_t c = 0;

  uint8_t *iPtr = NULL;
  uint8_t *basePtr = MAP_FAILED;

  size_t sizeBytes;

  int fd;
  struct stat st;

  int i, j, f, unquiet = 0;
  char *app;

  char *filename;

  app = argv[0];

  i = j = 1;
  while (i < argc) {
    if (argv[i][0] == '-') {
      f = argv[i][j];
      switch (f) {
        case 'v' :
          unquiet = 1;
          j++;
          break;

        case 'h' :
          usage(argv[0]);
          return EX_OK;

        case 'f' :
          j++;
          if (argv[i][j] == '\0') {
            j = 0;
            i++;
          }

          if (i >= argc) {
            printf("%s: usage: option -%c needs a parameter\n", app, f);
            return EX_USAGE;
          }

          switch(f){
            case 'f' :
              filename = argv[i] + j;
              break;
          }

          i++;
          j = 1;
          break;

        case '-' :
          j++;
          break;

        case '\0':
          j = 1;
          i++;
          break;

        default:
          fprintf(stderr, "%s: usage: unknown option -%c\n", app, argv[i][j]);
          return EX_USAGE;
      }
    } else {
      i++;
    }
  }

  fd = open(filename, O_RDONLY);
  if(fd < 0){
    fprintf(stderr, "unable to open %s: %s\n", filename, strerror(errno));
    return -1;
  }

  if(fstat(fd, &st) < 0){
    fprintf(stderr, "unable to stat %s: %s\n", filename, strerror(errno));
    close(fd);
    return -1;
  }

  sizeBytes = st.st_size;

  if (unquiet){
    printf("file %s has %lu bytes\n", filename, st.st_size);
  }

  basePtr = mmap(NULL, sizeBytes, PROT_READ, MAP_PRIVATE, fd, 0);
  if(basePtr == MAP_FAILED){
    fprintf(stderr, "unable to map %s: %s\n", filename, strerror(errno));
    close(fd);
    return -1;
  }

  if (unquiet){
    printf("mapped %s at %p\n", filename, basePtr);
  }

  close(fd);

  for (iPtr = basePtr; iPtr < (basePtr + sizeBytes); iPtr++){
    if (unquiet){
      printf("@%p  %08x\n", iPtr, *iPtr);
    }
    a = a + (uint16_t) *iPtr;
    b = b + a;
  }

  a = a % MODULO16;
  b = b % MODULO16;

  c = (b << 16) | a;

  printf("checksum is %X\n", c);

  return 0;
}
