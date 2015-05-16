#include <stdio.h>
#include <stdlib.h>

#include "errhandler.h"

void fatal(char *msg) {
  printf("%s\nExiting\n", msg);
  exit(EXIT_FAILURE);
}

void checkStatus(int status) {

  int child_status;

  if(WIFEXITED(status)) {
    child_status = WEXITSTATUS(status);
    if(0 != child_status) {
      fprintf(stderr, "Child exited with exit status: %d\n", child_status);
    }
  } else {
    if(WIFSIGNALED(status)) {
      /*child_ret = WTERMSIG(status);*/
      fatal("Child process was terminated by a signal ");
    }
  }
}
