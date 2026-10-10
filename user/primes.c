#include "kernel/types.h"
#include "user/user.h"

#define LIMIT 280

// Each stage receives the read end of the pipe fed by the previous stage.
// The first number that reaches a stage is the prime it stands for; every
// later number is passed on only if it is not a multiple of that prime, so
// each stage strips one prime's multiples from the pipeline. A stage never
// returns: it ends once its input is drained and its child is done.
//
// Closing the inherited fds is what makes the pipeline terminate. A stage ends
// its input only when every write end of that pipe is gone, so each stage may
// keep the write end of its own pipe and must drop both that end and its input
// before handing the read end to its child.
void primes(int input) __attribute__((noreturn));

int
main(int argc, char *argv[])
{
  int p[2];

  if (pipe(p) < 0) {
    printf("primes: pipe failed\n");
    exit(1);
  }

  if (fork() == 0) {
    close(p[1]);  // the first stage only reads
    primes(p[0]);
  }

  close(p[0]);
  for (int i = 2; i <= LIMIT; i++)
    write(p[1], &i, sizeof(int));
  close(p[1]);

  // Wait for the whole pipeline to drain before exiting, otherwise primes that
  // have not been reported yet would be lost.
  wait(0);
  exit(0);
}

void
primes(int input)
{
  int p[2];
  int prime;
  int mine = 0;

  if (pipe(p) < 0)
    exit(1);

  if (fork() == 0) {
    close(p[1]);
    close(input);
    primes(p[0]);
  }

  close(p[0]);

  // A short read means the pipe was closed with a half number still in flight,
  // which is how the pipeline signals its end.
  while (read(input, &prime, sizeof(int)) == sizeof(int)) {
    if (mine == 0) {
      mine = prime;
      printf("prime %d\n", mine);
    } else if (prime % mine != 0) {
      write(p[1], &prime, sizeof(int));
    }
  }

  close(p[1]);
  close(input);
  wait(0);
  exit(0);
}