#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define BUFSIZE 1024

// cp src dst: copy a file byte for byte through a fixed size buffer.
int
main(int argc, char *argv[])
{
  if (argc < 3) {
    printf("usage: cp src dst\n");
    exit(1);
  }

  int src = open(argv[1], O_RDONLY);
  if (src < 0) {
    printf("cp: cannot open %s\n", argv[1]);
    exit(1);
  }

  int dst = open(argv[2], O_CREATE | O_WRONLY | O_TRUNC);
  if (dst < 0) {
    printf("cp: cannot open %s\n", argv[2]);
    close(src);
    exit(1);
  }

  char buf[BUFSIZE];
  int n;
  while ((n = read(src, buf, BUFSIZE)) > 0) {
    int written = 0;
    while (written < n) {
      int w = write(dst, buf + written, n - written);
      if (w <= 0) {
        printf("cp: write error\n");
        close(src);
        close(dst);
        exit(1);
      }
      written += w;
    }
  }

  if (n < 0) {
    printf("cp: read error\n");
    close(src);
    close(dst);
    exit(1);
  }

  close(src);
  close(dst);
  exit(0);
}