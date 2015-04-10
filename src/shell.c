#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifndef __APPLE__
  #include <bits/sigaction.h> /* needed to use sigaction */
#endif

/* Background proess termination detection using signal handlers(1), or polling(0) */
#define SIGNALDETECTION       0

#define MAX_INPUT_CHARS       100
#define MAX_DIRECTORY_CHARS   300
#define MAX_CMD_ARGS          10

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
  struct sigaction saChild;
  
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
  if(strcmp(cmd, "exit\n") == 0) exit(0);    
  
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
  
  if(2 == cmdArgc /* no args to checkEnv */) {
  
  } else {
  
  }
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
  pid_t pid;
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
    if (foreground) waitpid(pid, NULL, 0);
    else {
      /*TODO*/
    }
  }

  return 0;
}

