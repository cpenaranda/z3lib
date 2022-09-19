// #define D(x) x

#ifndef D
#define D(x)
#else
#endif

#include "z3blib.c"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#undef D

int main(int argc, char *argv[])
{
  __u8 a[4], b[3];
  int i;
  __u32 c[Z3BE_HASHSIZE];
  a[0] = a[1] = a[2] = a[3] = 0;
  i = Z3BE_HASHSIZE;
  do {
    i -= 1;
    c[i] = 0;
  } while (i > 0);
  do {
    do {
      do {
        __s16 h, i;
        h = z3be_hash(&a[0]);
        c[h] += 1;
        i = z3be_hash(&a[1]);
        if (!((i ^ h) & 0xFF00)) {
          b[0] = a[1];
          b[1] = a[2];
          b[2] = i ^ h;
          printf("same hash: %02x %02x %02x %02x (%04x, %04x)\n",
            a[0], a[1], a[2], (__u8)(i ^ h), h, z3be_hash(&b[0]));
        }
      } while (++a[0] != 0);
    } while (++a[1] != 0);
  } while (++a[2] != 0);
  i = 0;
  do {
    fprintf(stderr, "%4lu\n", c[i]);
  } while (++i < Z3BE_HASHSIZE);
  return 0;
}
