// #define D(x) x

#ifndef D
#define D(x)
#else
#endif

#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "z3blib.c"
#include "z3crc32.h"

#undef D

int main(void)
{
  struct z3be_handle *zh;
  struct z3be_weighing weight;
  int r, i, c, w, j;
  time_t tm;
  unsigned long crc, len, ito, tel;
  char data[4096];
  char code[4096];
  char mem[Z3BE_MEMSIZE_MIN + Z3BE_MEMSIZE_EXTRA3];
  unsigned long crctab[256];
  gen_crc32_table(&crctab[0]);
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
  zh = z3be_start(&mem[0], sizeof(mem), 1, 1);
#ifdef D
  fprintf(stderr, "z3be_start(%p, %d): %p\n", &mem[0], sizeof(mem), zh);
#endif
  if (zh == NULL) {
    fprintf(stderr, "z3be_start failed\n");
    return 1;
  }
  r = 0;
  i = 0;
  do {
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
      if (r > 0) {
        c = z3be_put(zh, &data[i], r-i);
#ifdef D
        fprintf(stderr, "z3be_put(%d): %d\n", r-i, c);
#endif
        if (c > 0) i += c;
      } else {
        z3be_push(zh);
#ifdef D
        fprintf(stderr, "z3be_push\n");
#endif
      }
    } while ((r > 0) && (c > 0));
    tel = z3be_tell(zh, &weight, &ito);
#ifdef D
    fprintf(stderr, "z3be_tell=%lu, inpipe=%lu\n", tel, ito);
#endif
    do {
      c = z3be_get(zh, &weight, &code[0], sizeof(code));
#ifdef D
      fprintf(stderr, "z3be_get(%d): %d\n", sizeof(code), c);
#endif
      j = 0;
      while (j < c) {
        w = write(STDOUT_FILENO, &code[j], c-j);
        if (w <= 0) {
          fprintf(stderr, "write failed\n");
          return 1;
        }
        j += w;
      }
    } while (c > 0);
  } while (r > 0);
  c = z3be_finish(zh, &code[0]);
#ifdef D
  fprintf(stderr, "z3be_finish: %d\n", c);
#endif
  while (c > 0) {
    w = write(STDOUT_FILENO, &code[0], 1);
    if (w <= 0) {
      fprintf(stderr, "write failed\n");
      return 1;
    }
    c -= 8;
  }
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
