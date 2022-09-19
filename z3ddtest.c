#include "z3blib.c"
#include "z3dlib.c"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// #define D(x) x

#ifndef D
#define D(x)
#endif

// #define TEST_RELATIVE
// #define TEST_WITHOUTHEADER

static void badcode(char *s, unsigned long n)
{
  fprintf(stderr, "error: %s (%08lx)\n", s, n);
  exit(1);
}

static int readbyte(void)
{
  unsigned char c;
  int i;
  i = read(STDIN_FILENO, &c, 1);
  if (i != 1) return -1;
  return c;
}

int main(void)
{
  int i, j;
  __u32 pb;
  int pbnb;
  int error;
  __u8 *data;
  __u32 given;
  __u8 code[4096];
  __u8 *c;
  __u32 taken;
  struct z3dd_handle *zh;
#ifndef TEST_WITHOUTHEADER
  if (readbyte() != 0x1f) badcode("ID1 wrong", 0);
  if (readbyte() != 0x8b) badcode("ID2 wrong", 0);
  if (readbyte() != 8) badcode("unexpected compression method", 0);
  if (readbyte() != 0) badcode("cannot handle flags, sorry", 0);
  for (i = 0; i < 6; i++) readbyte();
#endif
  D(fprintf(stderr,"START\n");)
  zh = z3d_decode_init(0, 0, malloc(Z3DD_MEMSIZE), Z3DD_MEMSIZE);
  do {
    i = read(STDIN_FILENO, &code[0], sizeof(code));
    if (i <= 0) return 1;
    c = &code[0];
    do {
#ifdef TEST_RELATIVE
      { /* simulate garbage collection effect: */
        __u32 index;
        struct z3dd_handle *zh2;
        zh2 = malloc(Z3DD_MEMSIZE);
        if (!zh2) return 2;
        memcpy(zh2, zh, Z3DD_MEMSIZE);
        free(zh);
        zh = z3d_decode_relative(zh2, &error, &pb, &pbnb, c, i, &taken,
                &index, &given);
        data = index + (__u8*)zh;
D(fprintf(stderr,"DECODE/R: %p; index/data=%d/%08x, given=%lu, code-len=%d, taken=%lu\n", zh, index, data, given, i, taken);)
      }
#else
      zh = z3d_decode(zh, &error, &pb, &pbnb, c, i, &taken, &data, &given);
D(fprintf(stderr,"DECODE: %p; data=%08x, given=%lu, code-len=%d, taken=%lu\n", zh, data, given, i, taken);)
#endif
D(if ((data!=NULL) && (zh!=NULL)) fprintf(stderr,"DATA: %02x %02x...\n", data[0], data[1]);)
      if (zh != NULL) {
        while (given > 0) {
          j = write(STDOUT_FILENO, data, given);
          if ((j <= 0) || (j > given)) return 3;
          data += j;
          given -= j;
        }
        c += taken;
        i -= taken;
      }
    } while ((zh != NULL) && (i > 0));
  } while (zh != NULL);
  D(fprintf(stderr,"FINISH: error=%d, pb=%08lx, nb=%d\n", error, pb, pbnb);)
  return 0;
}
