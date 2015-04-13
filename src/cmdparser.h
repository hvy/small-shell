#ifndef SMALL_SHELL_CMD_PARSER_H
#define SMALL_SHELL_CMD_PARSER_H

/* Maximum number of command arguments supported  
 * including the commands */
#define MAX_CMD_ARGS          ( 10 )

/**
 * Splits input and adds each word in cmdArgv.
 * the function returns the number of words in cmdArgv 
 */
int parse(char *input, char *cmdArgv[], int *foreground);

#endif
