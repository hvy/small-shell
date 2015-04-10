#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Background proess termination detection using signal handlers(1), or polling(0) */
#define SIGNALDETECTION       0

#define MAX_INPUT_CHARS       100
#define MAX_DIRECTORY_CHARS   300
#define MAX_CMD_ARGS          10

void fatal(char *msg) {
  printf("%s\nExiting\n", msg);
  exit(EXIT_FAILURE);
}

void sigintHandler(int sigNum);

int parse(char *input, char *cmdArgv[], int *foreground);
int command(char *cmdArgv[], int foreground);


int main(int argc, char *argv[], char *envp[]) {
  
  size_t maxInputChars = MAX_INPUT_CHARS;
  int foreground;
  int cmdArgc;
  char *cmdArgv[MAX_CMD_ARGS];
  char *input = (char*) malloc(sizeof(char) * MAX_INPUT_CHARS);
  
  /* Will store the current working directory */
  char cwd[MAX_DIRECTORY_CHARS];
  
  /* Register signal handlers
  // TODO: handle different kill commands
  // SIGTERM should allow the process to clean up
  // SIGKILL cannot be caught and should just close the process directly
  // signal() is not recommended, use sigaction instead
  */
  if (signal(SIGINT, sigintHandler) == SIG_ERR) 
    fatal("Could not create signal handler for SIGINT");

  /* Main loop of the Shell */
  while(1) {
    getcwd(cwd, MAX_DIRECTORY_CHARS);
    /*printf("%s@%s>", getlogin(), cwd);*/
    printf("%d>", getpid());
    getline(&input, &maxInputChars, stdin);    

    /* Handle exit command */
    if(strcmp(input, "exit\n") == 0) {
      exit(0);    
    }
    
    /* Handle all other commands by first parsing the input string */
    cmdArgc = parse(input, cmdArgv, &foreground);

    if (strcmp(cmdArgv[0], "cd") == 0) {
      chdir(cmdArgv[1]); 
    } else {
      command(cmdArgv, foreground);
    } 
  } 

	return 0;
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

  return cmdArgc;
}

int command(char **cmdArgv, int foreground) {
  pid_t pid;
  printf("%s\n", (*cmdArgv)[0]);
  /*printf("%s\n", "command received");*/
  pid = fork();

  if (pid < 0) fatal("Failed to fork parent process");
  else if (0 == pid ) {
    /* Process is a child process */
    
    exit(EXIT_SUCCESS);
  } else {
    /* Process is a parent process */
  
  }

  return 0;
}

void sigintHandler(int sigNum) {
  /*TODO*/
  printf("sigint caught %d\n", sigNum);

  /*TODO
   * if not exiting, need to reinstall as signal handler again*/

  exit(0);
}
