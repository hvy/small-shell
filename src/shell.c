#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define MAX_INPUT_CHARS       80
#define MAX_DIRECTORY_CHARS   100
#define COMMAND_EXIT          "exit\n"


int main(int argc, char *argv[], char *envp[]) {

  /*execve("/bin/ls", argv, envp);*/

  size_t maxInputChars = MAX_INPUT_CHARS;
  char* input = (char*) malloc(sizeof(char) * MAX_INPUT_CHARS);

  char directory[MAX_DIRECTORY_CHARS];

  while(1) {
    getcwd(directory, MAX_DIRECTORY_CHARS);
    printf("%s@%s->", getlogin(), directory);
    getline(&input, &maxInputChars, stdin);    
  
    if(strcmp(input, COMMAND_EXIT) == 0) {
      exit(0);    
    }
  
  }

	return 0;
}
