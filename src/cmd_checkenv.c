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

void handleCheckEnvCmd(const int cmdArgc, char *cmdArgv[]) {
	/* File descriptors for the two pipes
	 * pipefd[0] read end of fst pipe, read by sort/grep
	 * pipefd[1] write end of fst pipe, written to by printenv
	 * pipefd[2] read end of snd pipe, read by pager
	 * pipefd[3] write end of snd pipe, written by sort/grep */
	int pipefd[4];
	int e, status;
  char *pager;
	pid_t childpid;

  /* Pager is set to NULL if PAGER isn't found. 
     If so, try less, else use pager */
	pager = getenv("PAGER");

	/* Create the two pipes */
  e = pipe(pipefd);
  if(-1 == e) fatal("Failed to create the fst pipe in checkEnv");
  e = pipe(pipefd + 2);
  if(-1 == e) fatal("Failed to create the snd pipe in checkEnv");
	
	/* printenv  */
	childpid = fork();
  if(0 > childpid) {
		fatal("Failed to fork by calling fork()");
	} else if(0 == childpid) {
    e = dup2(pipefd[1], STDOUT_FILENO);      
		if(-1 == e) fatal("Failed in printenv when calling dup2()");
    e = close(pipefd[0]);
    if(-1 == e) fatal("Failed in printenv when calling close()");
    e = close(pipefd[1]);
    if(-1 == e) fatal("Failed in printenv when calling close()");
    e = close(pipefd[2]);
    if(-1 == e) fatal("Failed in printenv when calling close()");
    e = close(pipefd[3]);
    if(-1 == e) fatal("Failed in printenv when calling close()");

		cmdArgv[0] = "printenv";
		cmdArgv[1] = NULL;
    execvp(cmdArgv[0], cmdArgv);

    fatal("Failed to execute printenv"); 
  }

	/* sort/grep */
	childpid = fork();
	if(0 > childpid) {
		fatal("Failed to fork by calling fork()");
	} else if(0 == childpid) {
    e = dup2(pipefd[0], STDIN_FILENO);  
		if(-1 == e) fatal("Failed in sort/grep when calling dup2()");
    e = dup2(pipefd[3], STDOUT_FILENO);  
		if(-1 == e) fatal("Failed in sort/grep when calling dup2()");
    e = close(pipefd[0]);
    if(-1 == e) fatal("Failed in sort/grep when calling close()");
    e = close(pipefd[1]);
    if(-1 == e) fatal("Failed in sort/grep when calling close()");
    e = close(pipefd[2]);
    if(-1 == e) fatal("Failed in sort/grep when calling close()");
    e = close(pipefd[3]);
    if(-1 == e) fatal("Failed in sort/grep when calling close()");

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
    e = dup2(pipefd[2], STDIN_FILENO);  
		if(-1 == e) fatal("Failed in printenv when calling dup2()");
    e = close(pipefd[0]);
    if(-1 == e) fatal("Failed in printenv when calling close()");
    e = close(pipefd[1]);
    if(-1 == e) fatal("Failed in printenv when calling close()");
    e = close(pipefd[2]);
    if(-1 == e) fatal("Failed in printenv when calling close()");
    e = close(pipefd[3]);
    if(-1 == e) fatal("Failed in printenv when calling close()");

		if(NULL != pager) {
			cmdArgv[0] = "pager";
		} else {
      cmdArgv[0] = "less";
		}
      	
		cmdArgv[1] = NULL;
		execvp(cmdArgv[0], cmdArgv);
   
    fatal("Failed to execute pager"); 
  }
	
	e = close(pipefd[0]);
  if(-1 == e) fatal("Failed in main process when calling close()");
  e = close(pipefd[1]);
  if(-1 == e) fatal("Failed in main process when calling close()");
	e = close(pipefd[2]);
  if(-1 == e) fatal("Failed in main process when calling close()");
  e = close(pipefd[3]);
  if(-1 == e) fatal("Failed in main process when calling close()");

	/* TODO Handle errors */
	#if !SIGNALDETECTION
		wait(&status);
		wait(&status);
		wait(&status);
	#endif
}

