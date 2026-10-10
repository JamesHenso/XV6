#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// du [path] [-a] [-s]
//
// A directory is reported as the sum of the sizes of everything below it; the
// inode size of the directory itself is deliberately left out so that the
// totals stay comparable between files and directories.

// Builds the path of one entry inside path into dst.
static char *
childpath(char *dst, char *path, char *name)
{
  int n = strlen(path);
  if (n + 1 + strlen(name) + 1 > 128)
    return 0;
  strcpy(dst, path);
  dst[n] = '/';
  strcpy(dst + n + 1, name);
  return dst;
}

// Returns the number of bytes under path. Directories are always reported,
// files only when showfiles is set, and nothing at all when quiet is set.
static long
du(char *path, int showfiles, int quiet)
{
  char child[128];
  struct dirent de;
  struct stat st;
  long total = 0;

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "du: cannot open %s\n", path);
    return 0;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "du: cannot stat %s\n", path);
    close(fd);
    return 0;
  }

  if (st.type != T_DIR) {
    close(fd);
    if (showfiles && !quiet)
      printf("%ld\t%s\n", st.size, path);
    return st.size;
  }

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;

    char name[DIRSIZ + 1];
    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = 0;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;

    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof child) {
      fprintf(2, "du: path too long\n");
      break;
    }

    total += du(childpath(child, path, name), showfiles, quiet);
  }
  close(fd);

  if (!quiet)
    printf("%ld\t%s\n", total, path);
  return total;
}

int
main(int argc, char *argv[])
{
  char *path = ".";
  int showfiles = 0;
  int summary = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-a") == 0) {
      showfiles = 1;
    } else if (strcmp(argv[i], "-s") == 0) {
      summary = 1;
    } else if (argv[i][0] == '-') {
      printf("usage: du [path] [-a] [-s]\n");
      exit(1);
    } else {
      path = argv[i];
    }
  }

  // Without -s, du() has already printed every line it owes, including the
  // root; printing the total here as well would duplicate that last line.
  if (summary)
    printf("%ld\t%s\n", du(path, 0, 1), path);
  else
    du(path, showfiles, 0);
  exit(0);
}