#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAXLINE 512

// diff file1 file2 [-q]
//
// Reads both files line by line and reports the lines that are not in both.
// The terminating newline is not part of a line, so "abc" and "abc\n" compare
// equal; that keeps files with and without a trailing newline from reporting a
// difference nobody asked about.

// Reads one line into buf, dropping the newline. Returns the length of the
// line, or -1 once the file is exhausted. An empty line is length 0 and is
// therefore not confused with end of file.
static int
readline(int fd, char *buf, int max)
{
  int i = 0;
  int got = 0;
  char c;

  while (i < max - 1) {
    if (read(fd, &c, 1) != 1)
      break;
    got = 1;
    if (c == '\n')
      break;  // falls through to the terminator, so the line is never left
              // unterminated for printf to run past
    buf[i++] = c;
  }
  buf[i] = 0;
  return got ? i : -1;
}

int
main(int argc, char *argv[])
{
  int quiet = 0;
  char *files[2] = {0, 0};

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-q") == 0) {
      quiet = 1;
    } else if (argv[i][0] == '-' && argv[i][1] != 0) {
      printf("usage: diff file1 file2 [-q]\n");
      exit(1);
    } else if (files[0] == 0) {
      files[0] = argv[i];
    } else if (files[1] == 0) {
      files[1] = argv[i];
    } else {
      printf("usage: diff file1 file2 [-q]\n");
      exit(1);
    }
  }

  if (files[0] == 0 || files[1] == 0) {
    printf("usage: diff file1 file2 [-q]\n");
    exit(1);
  }

  int fd1 = open(files[0], O_RDONLY);
  if (fd1 < 0) {
    printf("diff: cannot open %s\n", files[0]);
    exit(1);
  }
  int fd2 = open(files[1], O_RDONLY);
  if (fd2 < 0) {
    printf("diff: cannot open %s\n", files[1]);
    close(fd1);
    exit(1);
  }

  char l1[MAXLINE], l2[MAXLINE];
  int lineno = 1;
  int differ = 0;

  for (;;) {
    int n1 = readline(fd1, l1, sizeof(l1));
    int n2 = readline(fd2, l2, sizeof(l2));
    if (n1 < 0 && n2 < 0)
      break;

    if (n1 == n2 && strcmp(l1, l2) == 0) {
      lineno++;
      continue;
    }

    differ = 1;
    if (quiet)
      break;

    if (n1 < 0)
      printf("%s:%d: < EOF\n", files[0], lineno);
    else
      printf("%s:%d: < %s\n", files[0], lineno, l1);

    if (n2 < 0)
      printf("%s:%d: > EOF\n", files[1], lineno);
    else
      printf("%s:%d: > %s\n", files[1], lineno, l2);

    lineno++;
  }

  if (quiet && differ)
    printf("diff: files differ\n");

  close(fd1);
  close(fd2);
  exit(0);
}