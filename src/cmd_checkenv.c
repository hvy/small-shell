#define _XOPEN_SOURCE 500 /* needed to use sighold/sigrlse */
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

/* Creates a pipe for the given pipe file descriptor and takes care of the
  error handling */
void tryToPipe(int* fdv) {
  if(-1 == pipe(fdv)) {
    fatal("[ERROR] Failed to create a pipe in checkEnv");
  }
}

/* Closes the given file descriptor and takes care of the error handling */
void tryToClose(int fd) {
  if(-1 == close(fd)) {
    fatal("[ERROR] Failed to close a file descriptor");
	}
}

/* Copies oldfd file descriptor to newfd and takes care of the error handling */
void tryToDup2(int oldfd, int newfd) {
  if(-1 == dup2(oldfd, newfd)) {
    fatal("[ERROR] Failed to copy a file descriptor using dup2()");
  }
}

/* Waits for the given pid (blocking) and takes care of the error handling */
void tryToWait(pid_t pid) {
  int status;
  if(-1 == waitpid(pid, &status, 0)) {
      fatal("[ERROR] Failed to wait in checkEnv");
  }
  checkStatus(status);
}

void handleCheckEnvCmd(const int cmdArgc, char *cmdArgv[]) {

	int fstPipeFd[2], sndPipeFd[2], trdPipeFd[3];
	char *pager;
	pid_t pid;
  
  sighold(SIGCHLD);
  
  /* Pager is set to NULL if PAGER isn't found. 
     If so, try less, else use pager */
	pager = getenv("PAGER");
	
  /* printenv  */
  tryToPipe(fstPipeFd);

	pid = fork();

  if(0 > pid) {
    fatal("[ERROR] Failed to fork by calling fork()");
	} else if(0 == pid  /* Child process */) {
   	
    tryToDup2(fstPipeFd[1], STDOUT_FILENO);      
    tryToClose(fstPipeFd[0]);
    tryToClose(fstPipeFd[1]);
		
    /* Run printenv */
    cmdArgv[0] = "printenv";
		cmdArgv[1] = NULL;
    execvp(cmdArgv[0], cmdArgv);
   
    fatal("[ERROR] Failed to execute printenv"); 

  } else /* Parent process */ {
    
    tryToClose(fstPipeFd[1]);
    tryToWait(pid);
  }

  /* grep */
	tryToPipe(sndPipeFd);

  pid = fork();
	
  if(0 > pid) {
    fatal("[ERROR] Failed to fork by calling fork()");
	} else if(0 == pid /* Child process */ ) {
    
    tryToDup2(fstPipeFd[0], STDIN_FILENO);  
		tryToDup2(sndPipeFd[1], STDOUT_FILENO);
    tryToClose(sndPipeFd[0]);
    tryToClose(sndPipeFd[1]); 

    /* Run grep */  
    cmdArgv[0] = "grep";

    if(cmdArgc == 2 /* if no args are passed to checkEnv, ignore grep by letting it match any string */) {
      cmdArgv[1] = ".";
      cmdArgv[2] = NULL;
    }

    execvp(cmdArgv[0], cmdArgv);
   
    fatal("[ERROR] Failed to execute grep");
     
  } else /* Parent process */ {

    tryToClose(fstPipeFd[0]);
    tryToClose(sndPipeFd[1]);	
    tryToWait(pid);
  }
  
	/* sort */
	tryToPipe(trdPipeFd);
	
  pid = fork();
  
  if(0 > pid) {
     fatal("[ERROR] Failed to fork by calling fork()");
	} else if(0 == pid /* Child process */) {

    tryToDup2(sndPipeFd[0], STDIN_FILENO); 
  	tryToDup2(trdPipeFd[1], STDOUT_FILENO);
    tryToClose(trdPipeFd[0]);
    tryToClose(trdPipeFd[1]);    	
    
    /* Run sort */	
    cmdArgv[0] = "sort";
    cmdArgv[1] = NULL;
  
    execvp(cmdArgv[0], cmdArgv);
  
    fatal("[ERROR] Failed to execute sort/grep"); 
  
  } else /* Parent process */ {
    
    tryToClose(sndPipeFd[0]);	
    tryToClose(trdPipeFd[1]);	
    tryToWait(pid);
  }
	
	/* pager */
	pid = fork();
	
  if(0 > pid) {
		fatal("[ERROR] Failed to fork by calling fork()");
	} else if(0 == pid /* Child process */) {
    
    tryToDup2(trdPipeFd[0], STDIN_FILENO);  
	  tryToClose(trdPipeFd[0]);
    
    /* Run pager */	
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

    fatal("[ERROR] Failed to execute pager"); 
  
  } else /* Parent process */ {
    
    tryToClose(trdPipeFd[0]);
    tryToWait(pid);
  }
  
  sigrelse(SIGCHLD);
}


