#include "kernel/types.h"
#include "user/user.h"

// Pass a single byte back and forth between parent and child over two pipes:
// the child reports the ping, the parent reports the pong.
int
main(int argc, char *argv[])
{
  int ping[2], pong[2];
  char buf[1];

  if (pipe(ping) < 0 || pipe(pong) < 0) {
    printf("pingpong: pipe failed\n");
    exit(1);
  }

  int pid = fork();
  if (pid < 0) {
    printf("pingpong: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // Child: only reads from ping and writes to pong. Dropping the other
    // two ends keeps the parent's write from ever blocking on a full pipe.
    close(ping[1]);
    close(pong[0]);

    if (read(ping[0], buf, 1) != 1)
      printf("pingpong: child read failed\n");
    else
      printf("%d: received ping\n", getpid());

    if (write(pong[1], "x", 1) != 1)
      printf("pingpong: child write failed\n");

    close(ping[0]);
    close(pong[1]);
    exit(0);
  }

  // Parent: the mirror image. Wait until the child is done so that it cannot
  // become a zombie and so that pingpong never returns before its output.
  close(ping[0]);
  close(pong[1]);

  if (write(ping[1], "x", 1) != 1)
    printf("pingpong: parent write failed\n");

  if (read(pong[0], buf, 1) != 1)
    printf("pingpong: parent read failed\n");
  else
    printf("%d: received pong\n", getpid());

  close(ping[1]);
  close(pong[0]);
  wait(0);
  exit(0);
}