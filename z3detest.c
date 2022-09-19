// #define D(x) x

#ifndef D
#define D(x)
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "z3blib.c"
#define D(x) //x
#include "z3dlib.c"
#include "z3crc32.h"

#undef D

int main(int argc, char *argv[])
{
  struct z3de_handle *zh;
  int r, i, w, j, preferlonger, limitlength3;
  __u32 t, g, initialgrant;
  double thrmin, thrmax;
  time_t tm;
  unsigned long crc, len;
  unsigned long memsize, slice;
  char data[4096];
  char code[4096];
  char *mem;
  char *endptr;
  unsigned long crctab[256];
  gen_crc32_table(&crctab[0]);
  if (argc != 8) {
    fprintf(stderr, "usage: %s <extra memory> <slice size> <threshold(min)> <threshold(max)>\n\t\t<initial grant> <preferlonger> <limitlength3>\n", argv[0]);
    return 1;
  }
  memsize = atol(argv[1]);
  slice = atol(argv[2]);
  thrmin = strtod(argv[3], &endptr);
  if ((endptr == argv[3]) || (thrmin < 0) || (thrmin > 1)) {
    fprintf(stderr, "invalid threshold(%s)\n", "min");
    return 1;
  }
  thrmax = strtod(argv[4], &endptr);
  if ((endptr == argv[4]) || (thrmax < 0) || (thrmax > 1)) {
    fprintf(stderr, "invalid threshold(%s)\n", "max");
    return 1;
  }
  initialgrant = atol(argv[5]);
  preferlonger = atol(argv[6]);
  limitlength3 = atol(argv[7]);
  mem = malloc(Z3DE_MEMSIZE_MIN + memsize
                + (limitlength3 ? Z3BE_MEMSIZE_EXTRA3 : 0));
  crc = CRC_INIT_32;
  len = 0;
  data[0] = 0x1f;
  data[1] = 0x8b;
  data[2] = 0x08;
  data[3] = 0;
  tm = time(NULL);
  i = 4;
  do {
    data[i] = (unsigned char)tm;
    tm >>= 8;
  } while (++i < 8);
  data[8] = 0;
  data[9] = 255;
  i = 0;
  do {
    w = write(STDOUT_FILENO, &data[i], 10-i);
    if (w <= 0) {
      fprintf(stderr, "write failed\n");
      return 1;
    }
    i += w;
  } while (i < 10);
  zh = z3d_encode_init(mem, Z3DE_MEMSIZE_MIN + memsize
                + (limitlength3 ? Z3BE_MEMSIZE_EXTRA3 : 0), slice,
                thrmin * Z3DE_THRESHOLD_ONE, thrmax * Z3DE_THRESHOLD_ONE,
                initialgrant, preferlonger, limitlength3);
#ifdef D
  fprintf(stderr, "z3d_encode_init(%p, %lu, %lu): %p\n", mem, Z3DE_MEMSIZE_MIN + memsize + (limitlength3 ? Z3BE_MEMSIZE_EXTRA3 : 0), slice, zh);
#endif
  if (zh == NULL) {
    fprintf(stderr, "z3d_encode_init failed\n");
    return 1;
  }
  g = 0;
  r = 0;
  i = 0;
  do {
    if (i >= r) {
      r = read(STDIN_FILENO, &data[0], sizeof(data));
      if (r < 0) {
        fprintf(stderr, "read failed\n");
        return 1;
      } else if (r > 0) {
        crc = update_crc_32_block(&crctab[0], crc, &data[0], r);
        len += r;
      }
      i = 0;
    }
    zh = z3d_encode(zh, &data[i], r-i, &t, &code[0], sizeof(code), &g);
    i += t;
    j = 0;
    while (j < g) {
      w = write(STDOUT_FILENO, &code[j], g-j);
      if (w <= 0) {
        fprintf(stderr, "write failed\n");
        return 1;
      }
      j += w;
    }
  } while (zh != NULL);
  crc = ~crc;
  i = 0;
  do {
    data[i] = (unsigned char)crc;
    crc >>= 8;
  } while (++i < 4);
  do {
    data[i] = (unsigned char)len;
    len >>= 8;
  } while (++i < 8);
  i = 0;
  do {
    w = write(STDOUT_FILENO, &data[i], 8-i);
    if (w <= 0) {
      fprintf(stderr, "write failed\n");
      return 1;
    }
    i += w;
  } while (i < 8);
  return 0;
}
