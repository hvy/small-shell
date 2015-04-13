#ifndef SMALL_SHELL_CMD_CD_H_
#define SMALL_SHELL_CMD_CD_H_

/** 
 * Handle the cd command using chdir(). cmdArgv[0] is "cd", cmdArgv[1] is 
 * the dir. Note that the last element in cmdArgv is always NULL. 
 */
void handleCdCmd(const int cmdArgc, char *cmdArgv[]);

#endif
