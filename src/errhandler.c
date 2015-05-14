#include <stdio.h>
#include <stdlib.h>

#include "errhandler.h"

void fatal(char *msg) {
  printf("%s\nExiting\n", msg);
  exit(EXIT_FAILURE);
}

void checkStatus(int status) {

  int child_ret;

  if(WIFEXITED(status)) {
    child_ret = WEXITSTATUS(status);
    if(0 != child_ret) fatal("Bad exit status from child process");
  } else {
    if(WIFSIGNALED(status)) {
      /*child_ret = WTERMSIG(status);*/
      fatal("Child process was terminated by a signal ");
    }
  }
}
