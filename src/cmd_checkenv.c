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
	int e, c, status, fstPipeFd[2], sndPipeFd[2], trdPipeFd[3];
	char *pager;
	pid_t pid;
  
  sighold(SIGCHLD);
  
  /* Pager is set to NULL if PAGER isn't found. 
     If so, try less, else use pager */
	pager = getenv("PAGER");
	
  /* printenv  */
  e = pipe(fstPipeFd);
  if(-1 == e) {
    fatal("Failed to create the fst pipe in checkEnv");
  }

	pid = fork();

  if(0 > pid) {
    fatal("Failed to fork by calling fork()");
	} else if(0 == pid  /* Child process */) {
   	
    /* Close the read end of the snd pipe */
    c = close(fstPipeFd[0]);	
		if(-1 == c) {
      fatal("Failed to close fst pipe read");
	  }

    /* Duplicate the write end of the pipe to stdout */
    dup2AndHandleErr(fstPipeFd[1], STDOUT_FILENO);      
		
    /* Run printenv */
    cmdArgv[0] = "printenv";
		cmdArgv[1] = NULL;
    execvp(cmdArgv[0], cmdArgv);
   
    fatal("Failed to execute printenv"); 
  } else /* Parent process */ {
   	
    /* Close the write end of the fst pipe */
    c = close(fstPipeFd[1]);	
		if(-1 == c) {
      fatal("Failed to close a file descriptor in printenv");
	  }
    
    pid = waitpid(pid, &status, 0);
    
    if(-1 == pid) {
      fatal("Failed to wait in checkEnv");
    }
    checkStatus(status);
  }

  /* grep */
	e = pipe(sndPipeFd);
  if(-1 == e) {
    fatal("Failed to crete the snd pipe in checkEnv");
  }

  pid = fork();
	
  if(0 > pid) {
    fatal("Failed to fork by calling fork()");
	} else if(0 == pid /* Child process */ ) {
    
    /* Close the read end of the snd pipe */
    c = close(sndPipeFd[0]);
    if(-1 == c) {
      fatal("Failed to close a file descriptor in grep");
    }

    /* Duplicate the read end of the fst pipe to stdin */
    dup2AndHandleErr(fstPipeFd[0], STDIN_FILENO);  
    /* Duplicate the write end of snd pipe tp stdout */
		dup2AndHandleErr(sndPipeFd[1], STDOUT_FILENO);
      
    /* Run grep */  
    cmdArgv[0] = "grep";

    if(cmdArgc == 2 /* if no args are passed to checkEnv, ignore grep by letting it match any string */) {
      cmdArgv[1] = ".";
      cmdArgv[2] = NULL;
    }

    execvp(cmdArgv[0], cmdArgv);
   
    fatal("Failed to execute grep"); 
  } else /* Parent process */ {
    
    /* Close the read end of the fst pipe */
    c = close(fstPipeFd[0]);	
		if(-1 == c) {
      fatal("Failed to close a file descriptor in grep");
	  }
  	
    /* Close the write end of the snd pipe */
    c = close(sndPipeFd[1]);	
		if(-1 == c) {
      fatal("Failed to close a file descriptor in grep");
	  }
    
    pid = waitpid(pid, &status, 0);

    if(-1 == pid) {
      fatal("Failed to wait in checkEnv");
    }
  
    checkStatus(status);
  }
  
	/* sort */
	e = pipe(trdPipeFd);
  if(-1 == e) {
    fatal("Failed to crete the trd pipe in checkEnv");
  }
	
  pid = fork();
  
  if(0 > pid) {
     fatal("Failed to fork by calling fork()");
	} else if(0 == pid /* Child process */) {

    /* Close the read end of the trd pipe */
    c = close(trdPipeFd[0]);
    if(-1 == c) {
      fatal("Failed to close a file descriptor in sort");
    }
    
    /* Duplicate the read end of the snd pipe to stdin */
    dup2AndHandleErr(sndPipeFd[0], STDIN_FILENO); 
	  /* Duplicate the write end of the trd pipe to stdout */
  	dup2AndHandleErr(trdPipeFd[1], STDOUT_FILENO);
	
    /* Run sort */	
    cmdArgv[0] = "sort";
    cmdArgv[1] = NULL;
  
    execvp(cmdArgv[0], cmdArgv);
  
    fatal("Failed to execute sort/grep"); 
  } else /* Parent process */ {
    
    /* Close the read end of the snd pipe */
    c = close(sndPipeFd[0]);	
		if(-1 == c) {
      fatal("Failed to close a file descriptor in grep");
	  }
  	
    /* Close the write end of the trd pipe */
    c = close(trdPipeFd[1]);	
		if(-1 == c) {
      fatal("Failed to close a file descriptor in grep");
	  }
    
    pid = waitpid(pid, &status, 0);

    if(-1 == pid) {
      fatal("Failed to wait in checkEnv");
    }
    checkStatus(status);
  }
	
	/* pager */
	pid = fork();
	
  if(0 > pid) {
		fatal("Failed to fork by calling fork()");
	} else if(0 == pid /* Child process */) {
    
    /* Duplicate the read end of the trd pipe to stdin */ 
    dup2AndHandleErr(trdPipeFd[0], STDIN_FILENO);  
	
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

    fatal("Failed to execute pager"); 
  } else /* Parent process */ {
    
    /* Close the read end of the trd pipe */
    c = close(trdPipeFd[0]);
    if(-1 == c) {
      fatal("Failed to close a file descriptor in pager");
    }
    
    pid = waitpid(pid, &status, 0);
    if(-1 == pid) {
      fatal("Failed to wait in checkEnv");
    }
    checkStatus(status);
  }
  
  sigrelse(SIGCHLD);
}
