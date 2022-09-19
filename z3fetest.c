
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// #define D(x) x

#ifndef D
#define D(x)
#endif

#include <sys/time.h>
#include "z3blib.c"
#include "z3dlib.c"
#include "z3flib.c"

int main(int argc, char **argv)
{
  struct z3fe_handle zh;
  __u8 code[4096];
  struct timeval tv;
  int r, i, w;
  gettimeofday(&tv, NULL);
  r = z3f_encode_init(STDOUT_FILENO, &zh, (argc > 1) ? atoi(argv[1]) : 0,
        tv.tv_sec, 3, 0, NULL, (argc > 2) ? argv[2] : NULL, NULL);
  if (r < 0) {
    fprintf(stderr, "z3f_encode_init failed: %s\n", strerror(-r));
    return 1;
  }
  do {
    r = z3f_encode_header(&zh, 0, NULL, (argc > 2) ? argv[2] : NULL, NULL);
  } while (r == -EAGAIN);
  if (r < 0) {
    fprintf(stderr, "z3f_encode_header failed: %s\n", strerror(-r));
    return 1;
  }
  do {
    r = read(STDIN_FILENO, &code[0], sizeof(code));
    if (r < 0) {
      perror("read failed");
      return 1;
    }
    i = 0;
    while (i < r) {
      w = z3f_encode_write(&zh, &code[i], r-i);
      if (w == -EAGAIN) {
        w = 0;
      } else if (w <= 0) {
        fprintf(stderr, "z3f_encode_write failed: %d, %s\n", w, strerror(-w));
        return 1;
      }
      i += w;
    }
  } while (r != 0);
  do {
    w = z3f_encode_write(&zh, &code[0], 0);
  } while (w == -EAGAIN);
  if (w != 0) {
    fprintf(stderr, "z3f_encode_write failed: %d, %s\n", w, strerror(-w));
    return 1;
  }
  return 0;
}
