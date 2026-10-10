#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i = 1;
  int escape = 0;

  if(argc > 1 && strcmp(argv[1], "-e") == 0){
    escape = 1;
    i = 2;
  }

  for(; i < argc; i++){
    char *s = argv[i];
    int len = strlen(s);
    int start = 0;
    int end = len;

    if(len >= 2 && s[0] == '"' && s[len - 1] == '"'){
      start = 1;
      end = len - 1;
    }

    for(int j = start; j < end; j++){
      if(escape && s[j] == '\\' && j + 1 < end && s[j + 1] == 'n'){
        write(1, "\n", 1);
        j++;
      } else {
        write(1, &s[j], 1);
      }
    }

    if(i + 1 < argc){
      write(1, " ", 1);
    } else {
      write(1, "\n", 1);
    }
  }
  exit(0);
}
