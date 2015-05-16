#define _XOPEN_SOURCE 500 /* needed to use sighold/sigrlse */
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "cmdparser.h"
#include "cmd_cd.h"
#include "cmd_checkenv.h"
#include "errhandler.h"

/* Background proess termination detection using signal handlers(1), or polling(0) */
#define SIGDET                ( 1 )
#define MAX_INPUT_CHARS       ( 80 )
#define MAX_DIRECTORY_CHARS   ( 200 )

/* Linked list for processes */
struct processNode {
  pid_t pid;
  struct processNode *next;
};
struct processNode *finishedProcesses = NULL;
struct processNode *lastFinishedProcess = NULL;

/* A char pointer to the user input to the shell */
char *input;

void sigchldHandler(int sig);
void sigintHandler(int sigNum);
void registerSignalHandlers();
void printCwd();
void printFinBgProcs();
void readCmd(char *cmd);
void handleCmd(char *cmd);
void handleOtherCmd(char *cmdArgv[], int foreground);
pid_t pollProcess();

int main(int argc, char *argv[], char *envp[]) {
 
  input = (char*) malloc(sizeof(char) * MAX_INPUT_CHARS);

  registerSignalHandlers();
    
	/* Main loop of the Shell */
  while(1) {
    printFinBgProcs();
    printCwd();
    readCmd(input);
    handleCmd(input);
  } 

  free(input);

  return 0;
}

/* Print the pid of finished background processes on each new user prompt */
void printFinBgProcs() {
  
  int i = 1;

  #if SIGDET /* If using signal detection, go through all registered finished processes */
    
    struct processNode *toFree;
    struct processNode *pn = finishedProcesses;
    
    sighold(SIGCHLD);
    
    while (pn != NULL) {
      printf("[%d] %d\n", i++, pn -> pid);
      toFree = pn;
      pn = pn -> next;
      free(toFree);
    }
    
    finishedProcesses = NULL;
    lastFinishedProcess = NULL;

  #else /* If using polloing, poll using wait/waitpid*/ 
    
    pid_t fin_pid;
    
    sighold(SIGCHLD);
    
    while((fin_pid = pollProcess()) > 0) {
      printf("[%d] %d\n", i++, fin_pid);
    }

  #endif
  
  sigrelse(SIGCHLD);
}

pid_t pollProcess() {
	
	pid_t polledpid;

	polledpid = waitpid((pid_t) -1, NULL, WNOHANG);
  
  return polledpid;
}

void removeFinishedProcesses() {
  
  pid_t pid;
  int status;
  struct processNode *newNode;
 
  sighold(SIGCHLD);
  
  while ((pid = waitpid((pid_t) (-1), &status, WNOHANG | WUNTRACED)) > 0) {
    if (WIFEXITED(status)) {
  
      /* Add to list of finished processes */
      newNode = malloc(sizeof(struct processNode));
      if (newNode) {
        newNode -> pid = pid;
        newNode -> next = NULL;
        if (finishedProcesses == NULL) {
          finishedProcesses = newNode;
          lastFinishedProcess = newNode;
        } else {
          lastFinishedProcess -> next = newNode;
          lastFinishedProcess = newNode;
        }
      }
    }
  }
  
  sigrelse(SIGCHLD);
}

void sigintHandler(int sigNum) {
  signal(SIGINT, sigintHandler);
  fflush(stdout);
}

void sigchldHandler(int sig) {
  signal(SIGCHLD, sigchldHandler);
  removeFinishedProcesses();
}

void registerSignalHandlers() {
  signal(SIGINT, sigintHandler);
  
	#if SIGDET /* Register SIGCHLD if using signal detection */
  	signal(SIGCHLD, &sigchldHandler);
	#endif
}

/* Prints the current working directory to stdout */
void printCwd() {
  
  char *cwd;
  char cwdBuff[MAX_DIRECTORY_CHARS]; 

  cwd = getcwd(cwdBuff, MAX_DIRECTORY_CHARS);
  
  if(NULL == cwd) {
    fatal("[ERROR] Failed to get the current working directory using getcwd()");
  }

  /* Print the name of the current user and the workin directory */
	printf("%s@%s>", getlogin(), cwd);
}

/* Read the user input */
void readCmd(char *cmd) {
  
  char *e;
  int maxInputChars;
  
  maxInputChars = MAX_INPUT_CHARS;
  e = fgets(cmd, maxInputChars, stdin);
  
  if(NULL == e) {
    if (errno == EINTR) {
      readCmd(cmd);
    } else {
      fatal("[ERROR] Failed to read from stdin using fgets()");
    }
  }
}

/* Execute the given command */
void handleCmd(char *cmd) {
  
  /* foreground is set to 1 if the process is to be executed in the foreground
    of the shell (blocking). Else, it is set to 0 (non-blocking). It is set to 
    0 if the command is appended with a '&'.*/
  int foreground;
  int cmdArgc;
  char *cmdArgv[MAX_CMD_ARGS];
  
  /* Do nothing if the command is empty */
  if(1 == strlen(cmd)) {
    return;
  }

  /* Handle the exit command */
  if(strcmp(cmd, "exit\n") == 0) {
    free(input);
    kill(0, SIGTERM); /* Kill all processes for this process group */
    exit(0);    
  }

  /* Handle all other commands by first parsing the input string */
  cmdArgc = parse(cmd, cmdArgv, &foreground);
  
  if(0 == strcmp(cmdArgv[0], "cd")) {
    /* Jump to separate cd handler */
    handleCdCmd(cmdArgc, cmdArgv);
  } else if(0 == strcmp(cmdArgv[0], "checkEnv")) {
    /* Jump to separate checkEnv handler */
    handleCheckEnvCmd(cmdArgc, cmdArgv);
  } else { 
    /* Jump to the function taking care of all other commands */
    handleOtherCmd(cmdArgv, foreground); 
  }
}

/* Handles all commands except exit, cd and checkEnv. Those three commands are 
  handled by separate handlers. */
void handleOtherCmd(char *cmdArgv[], int foreground) {
  
  pid_t pid;
  int status;
  long running_time; /* foreground process running time in sec */
  struct timeval tv_start, tv_finish;

  sighold(SIGCHLD);
  
  pid = fork();
   
  if (pid < 0) { 
		fatal("[ERROR] Failed to fork parent process");
  } else if (0 == pid /* Child process */ ) {
    
    sigrelse(SIGCHLD);
   
    /* Execute the command */ 
    execvp(cmdArgv[0], cmdArgv);
    
    switch (errno) {
      case 2: 
        printf("command not found: %s\n", cmdArgv[0]);
        break;
      default:
        printf("[INFO] errno %d\n", errno);
    }
    
    exit(EXIT_SUCCESS);

  } else /* Parent process */{
    
    if (foreground) {

      /* Start measuring the running time for the foreground process */
      gettimeofday(&tv_start, NULL);
      
      /* Block the shell since the process is to be run in the foreground */
      pid = waitpid(pid, &status, 0);
      if(-1 == pid) {
        fatal("Failed during waitpid for the foreground process");
      }
      checkStatus(status);

      /* Stop measuring the running time of the foreground process and print it */
	    gettimeofday(&tv_finish, NULL);
      running_time = tv_finish.tv_sec - tv_start.tv_sec;
      printf("Running time of process %d: %lds\n", pid, running_time);
		}

    sigrelse(SIGCHLD);
  }
}
