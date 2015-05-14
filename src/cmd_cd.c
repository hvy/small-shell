#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "cmd_cd.h"

void handleCdCmd(const int cmdArgc, char *cmdArgv[]) { 
  int e;

  if(2 == cmdArgc /* handle root dir with 0 args to cd */)
    e = chdir(getenv("HOME"));
  else /* other dirs */
    e = chdir(cmdArgv[1]);

  if (0 != e /* failed to change dir,  change to home dir if chdir() fails */)
    printf("No such file or directory\n");
}

