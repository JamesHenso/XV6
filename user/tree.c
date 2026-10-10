#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// tree [path] [-L depth] [-d]
//
// The entry names of a directory are buffered rather than the directory read
// twice: readdir is a sequential stream, so there is no way to tell which
// entry is the last one without having seen them all.
//
// Two ceilings, both far above anything an xv6 filesystem holds: MAXENT caps
// the entries of a single directory, and the user stack is only USERSTACK
// pages, so each frame spends about 1.3 KB and the recursion tops out around
// five levels deep. If a real tree ever exceeds either, shrink the buffers to
// the same factor or move the scratch space off the stack.
#define MAXENT 32
#define MAXPATH 256

// Builds the path of one entry inside dir into dst, the way ls.c does it.
static char *
childpath(char *dst, char *dir, char *name)
{
  int n = strlen(dir);
  if (n + 1 + DIRSIZ + 1 > MAXPATH)
    return 0;
  strcpy(dst, dir);
  dst[n] = '/';
  memmove(dst + n + 1, name, DIRSIZ);
  dst[n + 1 + DIRSIZ] = 0;
  return dst;
}

// prefix is the indentation already emitted for this directory's children.
// childbuf must belong to this frame alone: the recursion reuses it for the
// path of each entry, and a shared buffer would overwrite the path the parent
// is still iterating over.
static void
tree(char *path, char *prefix, int depth, int maxdepth, int onlydir)
{
  char names[MAXENT][DIRSIZ + 1];
  char isdir[MAXENT];
  char next[MAXPATH];
  char childbuf[MAXPATH];
  char child[MAXPATH];
  struct dirent de;
  struct stat st;
  int n = 0;

  if (maxdepth >= 0 && depth >= maxdepth)
    return;

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "tree: cannot open %s\n", path);
    return;
  }

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;
    memmove(names[n], de.name, DIRSIZ);
    names[n][DIRSIZ] = 0;
    if (strcmp(names[n], ".") == 0 || strcmp(names[n], "..") == 0)
      continue;

    if (childpath(child, path, names[n]) == 0) {
      fprintf(2, "tree: path too long\n");
      break;
    }

    isdir[n] = stat(child, &st) == 0 && st.type == T_DIR;
    if (onlydir && !isdir[n])
      continue;

    n++;
    if (n == MAXENT) {
      fprintf(2, "tree: too many entries\n");
      break;
    }
  }
  close(fd);

  for (int i = 0; i < n; i++) {
    int last = i == n - 1;
    printf("%s%s %s\n", prefix, last ? "└──" : "├──", names[i]);

    if (isdir[i] && childpath(childbuf, path, names[i])) {
      strcpy(next, prefix);
      strcpy(next + strlen(prefix), last ? "    " : "│   ");
      tree(childbuf, next, depth + 1, maxdepth, onlydir);
    }
  }
}

int
main(int argc, char *argv[])
{
  char *path = ".";
  int maxdepth = -1;  // negative means unlimited
  int onlydir = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      onlydir = 1;
    } else if (strcmp(argv[i], "-L") == 0) {
      if (++i >= argc) {
        printf("usage: tree [path] [-L depth] [-d]\n");
        exit(1);
      }
      maxdepth = atoi(argv[i]);
    } else if (argv[i][0] == '-') {
      printf("usage: tree [path] [-L depth] [-d]\n");
      exit(1);
    } else {
      path = argv[i];
    }
  }

  printf("%s\n", path);
  tree(path, "", 0, maxdepth, onlydir);
  exit(0);
}