#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "cmd_checkenv.h"
#include "errhandler.h"

void closeFd(const int fdc, int *const fdv) {
	int c, i;
	for(i = 0; i < fdc; ++i) {
		c = close(fdv[i]);	
		if(-1 == c) fatal("Failed to close a file descriptor in closePipeFd()");
	}
}

void dup2AndHandleErr(int oldfd, int newfd) {
	int d;
	d = dup2(oldfd, newfd);      
	if(-1 == d) fatal("Failed to copy file descriptor using dup2(2) in dup2AndClose()");
}

void handleCheckEnvCmd(const int cmdArgc, char *cmdArgv[]) {
	/* File descriptors for the two pipes
	 * pipefd[0] read end of fst pipe, read by sort/grep
	 * pipefd[1] write end of fst pipe, written to by printenv
	 * pipefd[2] read end of snd pipe, read by pager
	 * pipefd[3] write end of snd pipe, written by sort/grep */
	int e, status, pipefdc = 4, pipefdv[4];
	char *pager;
	pid_t childpid;

  /* Pager is set to NULL if PAGER isn't found. 
     If so, try less, else use pager */
	pager = getenv("PAGER");

	/* Create the two pipes */
  e = pipe(pipefdv);
  if(-1 == e) fatal("Failed to create the fst pipe in checkEnv");
  e = pipe(pipefdv + 2);
  if(-1 == e) fatal("Failed to create the snd pipe in checkEnv");
	
	/* printenv  */
	childpid = fork();
  if(0 > childpid) fatal("Failed to fork by calling fork()");
	else if(0 == childpid) {
    dup2AndHandleErr(pipefdv[1], STDOUT_FILENO);      
		closeFd(pipefdc, pipefdv);

		cmdArgv[0] = "printenv";
		cmdArgv[1] = NULL;
    execvp(cmdArgv[0], cmdArgv);

    fatal("Failed to execute printenv"); 
  }

	/* sort/grep */
	childpid = fork();
	if(0 > childpid) fatal("Failed to fork by calling fork()");
	else if(0 == childpid) {
    dup2AndHandleErr(pipefdv[0], STDIN_FILENO);  
		dup2AndHandleErr(pipefdv[3], STDOUT_FILENO);
    closeFd(pipefdc, pipefdv);

		if(2 == cmdArgc) {
			cmdArgv[0] = "sort";
      cmdArgv[1] = NULL;
      execvp(cmdArgv[0], cmdArgv);
    } else {
      cmdArgv[0] = "grep";
      execvp(cmdArgv[0], cmdArgv);
    }
    
    fatal("Failed to execute sort/grep"); 
  }
	
	/* pager */
	childpid = fork();
	if(0 > childpid) {
		fatal("Failed to fork by calling fork()");
	} else if(0 == childpid) {
    dup2AndHandleErr(pipefdv[2], STDIN_FILENO);  
		closeFd(pipefdc, pipefdv);

		if(NULL != pager) {
			/* Try pager since it was found */
			cmdArgv[0] = "pager";
	   	cmdArgv[1] = NULL;
			execvp(cmdArgv[0], cmdArgv);
		} else {
		  /* Try less since pager wasn't found */
      cmdArgv[0] = "less";
	   	cmdArgv[1] = NULL;
			execvp(cmdArgv[0], cmdArgv);
		
			/* Try more since less failed */
			cmdArgv[0] = "more";
	   	cmdArgv[1] = NULL;
			execvp(cmdArgv[0], cmdArgv);
		}

    fatal("Failed to execute pager"); 
  }
	
	closeFd(pipefdc, pipefdv);

	/* TODO Handle errors */
	#if !SIGNALDETECTION
		wait(&status);
		wait(&status);
		wait(&status);
	#endif
}
