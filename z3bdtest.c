#include "z3blib.c"

#include <unistd.h>
#include <stdio.h>

// #define D(x) x

#ifndef D
#define D(x)
#endif

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
  int i, error;
  __u32 pb;
  unsigned char c, flags;
  unsigned char *d;
  unsigned long e0, e1, d0, d1;
  struct z3bd_handle *zh;
  static char mem[Z3BD_MEMSIZE];
  if (readbyte() != 0x1f) badcode("ID1 wrong", 0);
  if (readbyte() != 0x8b) badcode("ID2 wrong", 0);
  if (readbyte() != 8) badcode("unexpected compression method", 0);
  flags = readbyte();
  for (i = 0; i < 6; i++) readbyte();
  if (flags & (1 << 2)) {
    i = readbyte();
    i += readbyte() << 8;
    while (i-- > 0) readbyte();
  }
  if (flags & (1 << 3)) do c = readbyte(); while (c != 0);
  if (flags & (1 << 4)) do c = readbyte(); while (c != 0);
  if (flags & (1 << 1)) {
    readbyte();
    readbyte();
  }
  zh = z3bd_start(0, 0, &mem[0], sizeof(mem));
  D(fprintf(stderr, "START\n");)
  e0 = e1 = d0 = d1 = 0;
  do {
    do {
      while ((i = z3bd_get(zh, &d)) > 0) {
        int j = 0;
        do {
          D(fprintf(stderr, "GET: %02x\n", d[j]);)
          putchar(d[j]);
          d1 += 1;
        } while (++j < i);
      }
      if (d != NULL) {
        i = readbyte();
        D(fprintf(stderr, "IN: %d\n", i);)
        if (i >= 0) {
          c = i;
          i = z3bd_put(zh, &c, 1);
          D(fprintf(stderr, "PUT: %02x, %lu\n", c, i);)
          e1 += 1;
        } else {
          badcode("unexpected EOF", 0);
        }
      }
    } while (d != NULL);
    error = z3bd_finish(zh, &pb, &i);
    D(fprintf(stderr, "FINISH: %d, pb=%08lx, nb=%d, code %6lu/%6lu\n", error, pb, i, e1 - e0, d1 - d0);)
    d0 = d1;
    e0 = e1;
  } while (error == z3err_bd_notbfinal);
  return 0;
}
