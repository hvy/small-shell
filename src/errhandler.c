#include <stdio.h>
#include <stdlib.h>

#include "errhandler.h"

void fatal(char *msg) {
  printf("%s\nExiting\n", msg);
  exit(EXIT_FAILURE);
}
