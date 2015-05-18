#define _XOPEN_SOURCE 500 /* needed to use sighold/sigrlse */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "errhandler.h"

void fatal(char *msg) {
  printf("%s\nExiting\n", msg);
  exit(EXIT_FAILURE);
}

void checkStatus(int status) {

  int ret;

  if(WIFEXITED(status)) {
    ret = WEXITSTATUS(status);
    if(0 != ret) {
      fprintf(stderr, "[INFO] Child exited with exit code: %d\n", ret);
    }
  } else {
    if(WIFSIGNALED(status)) {
      ret = WTERMSIG(status);
      fprintf(stderr, "[INFO] Child process was terminated by signal: %d\n", ret);
    }
  }
}
