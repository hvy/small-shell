#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#ifndef __APPLE__
  #include <bits/sigaction.h> /* needed to use sigaction */
#endif

/* Background proess termination detection using signal handlers(1), or polling(0) */
#define SIGNALDETECTION       ( 0 )
#define MAX_INPUT_CHARS       ( 100 )
#define MAX_DIRECTORY_CHARS   ( 300 )
#define MAX_CMD_ARGS          ( 10 )
#define PIPE_READ_SIDE        ( 0 )
#define PIPE_WRITE_SIDE       ( 1 )

/* Linked list for processes */
struct processNode {
  pid_t pid;
  struct processNode *next;
};
struct processNode *headProcess = NULL;

void fatal(char *msg) {
  printf("%s\nExiting\n", msg);
  exit(EXIT_FAILURE);
}

void sigintHandler(int sigNum) {
  /*
   * TODO: send to foreground child
   */
  exit(0);
}

void sigchldHandler(int sig) {
  while (waitpid((pid_t) (-1), 0, WNOHANG) > 0);
}

void registerSignalHandlers() {
  struct sigaction sa;
  struct sigaction saChild;
  
  sa.sa_handler = &sigintHandler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGINT, &sa, 0) == -1)
    fatal("Could not create signal handlers for SIGINT");

  saChild.sa_handler = &sigchldHandler;
  sigemptyset(&saChild.sa_mask);
  /*saChild.sa_flags = SA_RESTART | SA_NOCLDSTOP;*/
	
	if (sigaction(SIGCHLD, &saChild, 0) == -1)
    fatal("Could not create signal handler for SIGCHLD");
}

int parse(char *input, char *cmdArgv[], int *foreground);
int command(char *cmdArgv[], int foreground);
void printCwd();
void readCmd(char *cmd);
void handleCmd(char *cmd);
void handleCd(const int cmdArgc, char *cmdArgv[]);
void handleCheckEnv(const int cmdArgc, char *cmdArgv[]);
void pollpid(const pid_t pid);

int main(int argc, char *argv[], char *envp[]) {
  
  char *input = (char*) malloc(sizeof(char) * MAX_INPUT_CHARS);

	#if SIGNALDETECTION == 1
  	registerSignalHandlers();
	#endif

	/* Main loop of the Shell */
  while(1) {
    printCwd();
    readCmd(input);
    handleCmd(input);
  } 

  free(input);

  return 0;
}

void printCwd() {
  char *cwd;
  char cwdBuff[MAX_DIRECTORY_CHARS]; 

  cwd = getcwd(cwdBuff, MAX_DIRECTORY_CHARS);
  
  if(NULL == cwd) 
    fatal("Failed to get the current working directory using getcwd()");
  printf("%s@%s>", getlogin(), cwd);
  /* printf("%d>", getpid()); */
}

void readCmd(char *cmd) {
  char *e;
  int maxInputChars;
  
  maxInputChars = MAX_INPUT_CHARS;
  e = fgets(cmd, maxInputChars, stdin);
  
  if(NULL == e)
    fatal("Failed to read from stdin using fgets()"); 
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
      kill(p->pid, SIGKILL); /* TODO: maybe SIGTERM is better here? */
      free(p);
    }

    exit(0);    
  }

  /* Handle all other commands by first parsing the input string */
  cmdArgc = parse(cmd, cmdArgv, &foreground);

  if(0 == strcmp(cmdArgv[0], "cd"))
    handleCd(cmdArgc, cmdArgv);
  else if(0 == strcmp(cmdArgv[0], "checkEnv"))
    handleCheckEnv(cmdArgc, cmdArgv);
  else 
    command(cmdArgv, foreground); 
}

/* Handle the cd command using chdir(). cmdArgv[0] is "cd", cmdArgv[1] is 
  the dir. Note that the last element in cmdArgv is always NULL. */
void handleCd(const int cmdArgc, char *cmdArgv[]) { 
  int e;

  if(2 == cmdArgc /* handle root dir with 0 args to cd */) {
    e = chdir("/");
  } else if(0 == strcmp(cmdArgv[1], "~") /* home dir */) {
    e = chdir(getenv("HOME"));
  } else /* other dirs */ {
    e = chdir(cmdArgv[1]);
  }

  if (0 != e /* failed to change dir,  change to home dir if chdir() fails */)
    e = chdir(getenv("HOME"));
}

/* Executes "printenv | sort | pager" if no arguments are given to the command.
   If arguments are passed to the command then 
   "printenv | grep <arguments> | sort | pager" is be executed. The pager 
   executed is selected primarily based on the value of the users "PAGER" 
   environment variable. If no such variable is set then it tries to execute 
   "less" and if that fails "more". */
void handleCheckEnv(const int cmdArgc, char *cmdArgv[]) {
	int e,  childStatus, status;
  int pipefd[2];
  char *pager;
  char *emptyArgv[2];
	pid_t childpid, fstchildpid, sndchildpid;

  /* Pager is set to NULL if PAGER isn't found. 
     If so, try less, else use pager */
	pager = getenv("PAGER");

  /* pipe */
  e = pipe(pipefd);
  if(-1 == e) fatal("Failed in checkEnv when calling pipe()");

	childpid = fork();
  if(0 > childpid) {
		fatal("Failed to fork by calling fork()");
	} else if(0 == childpid) {
    e = dup2(pipefd[PIPE_WRITE_SIDE], STDOUT_FILENO);  
    if(-1 == e) fatal("Failed in printenv when calling dup2()");
    e = close(pipefd[PIPE_WRITE_SIDE]);
    if(-1 == e) fatal("Failed in printenv when calling close()");
    e = close(pipefd[PIPE_READ_SIDE]);
    if(-1 == e) fatal("Failed in printenv when calling close()");

		cmdArgv[0] = "printenv";
		cmdArgv[1] = NULL;
    execvp(cmdArgv[0], cmdArgv);

    fatal("Failed to execute printenv"); 
  }

	fstchildpid = childpid;

	childpid = fork();
	if(0 > childpid) {
		fatal("Failed to fork by calling fork()");
	} else if(0 == childpid) {
    e = dup2(pipefd[PIPE_READ_SIDE], STDIN_FILENO);  
    if(-1 == e) fatal("Failed in sort/grep when calling dup2()");
     e = close(pipefd[PIPE_WRITE_SIDE]);
    if(-1 == e) fatal("Failed in sort/grep when calling close()");
    e = close(pipefd[PIPE_READ_SIDE]);
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
	
	sndchildpid = childpid;
	
  e = close(pipefd[PIPE_READ_SIDE]);
  if(-1 == e) fatal("Failed in sort/grep when calling close()");
  e = close(pipefd[PIPE_WRITE_SIDE]);
  if(-1 == e) fatal("Failed in sort/grep when calling close()");


	fprintf(stderr, "Waiting for pid %d\n", fstchildpid);
	pollpid(fstchildpid);
	fprintf(stderr, "done\n");

	fprintf(stderr, "Waiting for pid %d\n", sndchildpid);
	pollpid(sndchildpid);
	fprintf(stderr, "done\n");



}

/* Splits input and adds each word in cmdArgv.
    the function returns the number of words in cmdArgv */
int parse(char *const input, char *cmdArgv[], int *foreground) {

  char *token;
  int cmdArgc = 0;

  /* Remove the trailing \n */
  size_t ln = strlen(input) - 1;
  if ('\n' == input[ln]) {
    input[ln] = '\0';
  } 

  *foreground = 1;
  if (ln > 0) {
    /* & */
    if (38 == input[ln - 1]) {
      input[ln - 1] = '\0';
      *foreground = 0;
    }
  }

  token = strtok(input, " ");
  
  while (token != NULL && cmdArgc < MAX_CMD_ARGS) {
    cmdArgv[cmdArgc] = token;
    ++cmdArgc;
    token = strtok(NULL,  " ");
  }

  cmdArgv[cmdArgc] = NULL;
  ++cmdArgc;

  return cmdArgc;
}

int command(char **cmdArgv, int foreground) {
  pid_t pid, w;
  pid = fork();

  if (pid < 0) fatal("Failed to fork parent process");
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
			w = waitpid(pid, NULL, 0);
			printf("Wait ret: %d,\n", w);
		} else {
      struct processNode *newProcess = malloc(sizeof(struct processNode));
      newProcess -> pid = pid; 
      newProcess -> next = headProcess;
      headProcess = newProcess;
    }
  }

  return 0;
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

