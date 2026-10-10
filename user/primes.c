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
  int prime;

  // Read the first number from the incoming pipe.
  // If the pipe is empty (EOF), close input and exit cleanly without forking.
  if (read(input, &prime, sizeof(int)) <= 0) {
    close(input);
    exit(0);
  }

  // The first number reaching this stage is guaranteed to be prime.
  printf("prime %d\n", prime);

  int p[2];
  if (pipe(p) < 0) {
    fprintf(2, "primes: pipe failed\n");
    close(input);
    exit(1);
  }

  int pid = fork();
  if (pid < 0) {
    fprintf(2, "primes: fork failed\n");
    close(input);
    close(p[0]);
    close(p[1]);
    exit(1);
  }

  if (pid == 0) {
    // Child: next stage in the pipeline
    close(p[1]);
    close(input);
    primes(p[0]);
  } else {
    // Parent: filter remaining numbers and forward non-multiples to the child
    close(p[0]);
    int n;
    while (read(input, &n, sizeof(int)) == sizeof(int)) {
      if (n % prime != 0) {
        write(p[1], &n, sizeof(int));
      }
    }

    close(input);
    close(p[1]); // Signal EOF to child
    wait(0);
    exit(0);
  }
}