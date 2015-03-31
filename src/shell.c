#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

/* Background proceess termination detection using signal handlers(1), or polling(0) */
#define SIGNALDETECTION       0

#define MAX_INPUT_CHARS       100
#define MAX_DIRECTORY_CHARS   200
#define MAX_CMD_ARGS          10

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

  /* Main loop of the Shell */
  while(1) {
    getcwd(cwd, MAX_DIRECTORY_CHARS);
    printf("%s@%s>", getlogin(), cwd);
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
int parse(char* input, char *cmdArgv[], int *foreground) {
  
  char *token;
  int cmdArgc = 0;

  /* Remove the trailing \n */
  size_t ln = strlen(input) - 1;
  if ('\n' == input[ln]) {
    input[ln] = '\0';
  } 

  *foreground = 1;
  if (ln > 0) {
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

int command(char *cmdArgv[], int foreground) {

  pid_t pid = fork();

  if (pid < 0) { 
    /* Failed to fork parent process */
    exit(-1);
  } else if (0 == pid ) {
    /* Process is a child process */

  } else  {
    /* Process is a parent process */
  
  }

  return 0;
}
