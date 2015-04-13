#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifndef __APPLE__
  #include <bits/sigaction.h> /* needed to use sigaction */
#endif

#include "cmdparser.h"
#include "cmd_cd.h"
#include "cmd_checkenv.h"
#include "errhandler.h"

/* Background proess termination detection using signal handlers(1), or polling(0) */
#define SIGNALDETECTION       ( 0 )
#define MAX_INPUT_CHARS       ( 100 )
#define MAX_DIRECTORY_CHARS   ( 300 )

/* Linked list for processes */
struct processNode {
  pid_t pid;
  struct processNode *next;
};

struct processNode *headProcess = NULL;

void sigchldHandler(int sig);
void sigintHandler(int sigNum);
void registerSignalHandlers();
void removeZombieProcesses();
void printCwd();
void readCmd(char *cmd);
void handleCmd(char *cmd);
void handleOtherCmd(char *cmdArgv[], int foreground);
void pollpid(const pid_t pid);

int main(int argc, char *argv[], char *envp[]) {
  
  char *input = (char*) malloc(sizeof(char) * MAX_INPUT_CHARS);

  registerSignalHandlers();

	/* Main loop of the Shell */
  while(1) {
    printCwd();
    readCmd(input);
    handleCmd(input);
  } 

  free(input);

  return 0;
}

void sigintHandler(int sigNum) {
  /*
   * TODO: send SIGINT/SIGTERM to foreground child
   */
  signal(SIGINT, sigintHandler);
  fflush(stdout);
}

void sigchldHandler(int sig) {
  while (waitpid((pid_t) (-1), 0, WNOHANG) > 0);
}

void registerSignalHandlers() {
  signal(SIGINT, sigintHandler);
  
	#if SIGNALDETECTION
  	signal(SIGCHLD, &sigchldHandler);
	#endif
}

void removeZombieProcesses() {
  while (waitpid((pid_t) (-1), 0, WNOHANG) > 0);
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

  /* TODO Handle exit properly. Make sure no zombie processes are left. */
  /* Handle exit command */
  if(strcmp(cmd, "exit\n") == 0) {
    while (headProcess) {
      struct processNode *p = headProcess;
      headProcess = p->next;
      kill(p->pid, SIGTERM);
      free(p);
    }

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

void pollpid(const pid_t pid) {
	
	int status;
	pid_t polledpid;

	polledpid = waitpid(pid, &status, 0);

	if(-1 == polledpid) fatal("Failed to poll using waitpid()");
  if(WIFEXITED(status)) {
    polledpid = WEXITSTATUS(status);
    if(0 != polledpid) 
			fatal("Bad WEXITSTATUS in wait()");
  } else {
    if(WIFSIGNALED(status)) fatal("Process exited by signal in wait()");
  }
}

void handleOtherCmd(char *cmdArgv[], int foreground) {
  pid_t pid, w;
  pid = fork();

  if (pid < 0) 
		fatal("Failed to fork parent process");
  else if (0 == pid ) {
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
			/* TODO Handle error properly */
			w = waitpid(pid, NULL, 0);
			printf("Wait ret: %d,\n", w);
		} else {
      /* TODO Make sure malloced memory is freed */
			struct processNode *newProcess = malloc(sizeof(struct processNode));
      newProcess -> pid = pid; 
      newProcess -> next = headProcess;
      headProcess = newProcess;
    }
  }
}
