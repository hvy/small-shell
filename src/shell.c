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
#define SIGDET                ( 0 )
#define MAX_INPUT_CHARS       ( 100 )
#define MAX_DIRECTORY_CHARS   ( 300 )

/* Linked list for processes */
struct processNode {
  pid_t pid;
  struct processNode *next;
};
struct processNode *finishedProcesses = NULL;
struct processNode *lastFinishedProcess = NULL;

char *input;
char *WAIT_FG = 0;

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

void printFinBgProcs() {
  int i = 1;

  #if SIGDET
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

  #else /* Use polling by using wait/waitpid */ 
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
  
	#if SIGDET
  	signal(SIGCHLD, &sigchldHandler);
	#endif
}

void printCwd() {
  char *cwd;
  char cwdBuff[MAX_DIRECTORY_CHARS]; 

  cwd = getcwd(cwdBuff, MAX_DIRECTORY_CHARS);
  
  if(NULL == cwd)
    fatal("Failed to get the current working directory using getcwd()");

	printf("%s@%s>", getlogin(), cwd);
}

void readCmd(char *cmd) {
  char *e;
  int maxInputChars;
  
  maxInputChars = MAX_INPUT_CHARS;
  e = fgets(cmd, maxInputChars, stdin);
  
  if(NULL == e) {
    if (errno == EINTR) readCmd(cmd);
     else fatal("Failed to read from stdin using fgets()");
  }
}

void handleCmd(char *cmd) {
  int cmdArgc, foreground;
  char *cmdArgv[MAX_CMD_ARGS];
  
  /* Handle empty command */
  if(1 == strlen(cmd)) return;

  /* Handle exit command */
  if(strcmp(cmd, "exit\n") == 0) {
    kill(0, SIGTERM); /* Kill all processes for this process group */
    free(input);
    exit(0);    
  }

  /* Handle all other commands by first parsing the input string */
  cmdArgc = parse(cmd, cmdArgv, &foreground);

  if(0 == strcmp(cmdArgv[0], "cd"))
    handleCdCmd(cmdArgc, cmdArgv);
  else if(0 == strcmp(cmdArgv[0], "checkEnv"))
    handleCheckEnvCmd(cmdArgc, cmdArgv);
  else 
    handleOtherCmd(cmdArgv, foreground); 
}

void handleOtherCmd(char *cmdArgv[], int foreground) {
  pid_t pid, w;
  struct timeval tv_start, tv_finish;
  long running_time; /* foreground process running time in sec */

  sighold(SIGCHLD);
  pid = fork();
   
  if (pid < 0) 
		fatal("Failed to fork parent process");
  else if (0 == pid ) {
    sigrelse(SIGCHLD);
    /* Process is a child process */
    execvp(cmdArgv[0], cmdArgv);
    switch (errno) {
      case 2: 
        printf("command not found: %s\n", cmdArgv[0]);
        break;
      default:
        printf("errno %d\n", errno);
    }
    exit(EXIT_SUCCESS);
  } else {
    /* Process is a parent process */
    if (foreground) {
      gettimeofday(&tv_start, NULL);
      w = waitpid(pid, NULL, 0);
      sigrelse(SIGCHLD);
      /*removeFinishedProcesses();*/
	    gettimeofday(&tv_finish, NULL);
      running_time = tv_finish.tv_sec - tv_start.tv_sec;
      printf("Foreground process running time: %lds\n", running_time);
      printf("Wait ret: %d\n", w);
		}
  }
}
