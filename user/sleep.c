#include "kernel/types.h"
#include "user/user.h"

// sleep n: block the calling process for n clock ticks (~10ms per tick).
int
main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("usage: sleep n\n");
    exit(1);
  }

  sleep(atoi(argv[1]));
  exit(0);
}