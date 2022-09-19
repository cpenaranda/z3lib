
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// #define D(x) x

#ifndef D
#define D(x)
#endif

#include "z3blib.c"
#include "z3dlib.c"
#include "z3flib.c"

int main(void)
{
  struct z3fd_handle zh;
  __u8 code[4096];
  int r, i, w;
  r = z3f_decode_init(STDIN_FILENO, &zh);
  if (r < 0) {
    fprintf(stderr, "z3f_decode_init failed: %s\n", strerror(-r));
    return 1;
  }
  do {
    r = z3f_decode_read(&zh, &code[0], sizeof(code));
    if ((r < 0) && (r != -EAGAIN)) {
      fprintf(stderr, "z3f_decode_read failed: %s\n", strerror(-r));
      return 1;
    }
    i = 0;
    while (i < r) {
      w = write(STDOUT_FILENO, &code[i], r-i);
      if (w < 0) {
        perror("write failed");
        return 1;
      }
      i += w;
    }
  } while (r != 0);
  return 0;
}
