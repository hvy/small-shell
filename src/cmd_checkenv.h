#ifndef SMALL_SHELL_CMD_CHECKENV_H_
#define SMALL_SHELL_CMD_CHECKENV_H_

/** 
 * Executes "printenv | sort | pager" if no arguments are given to the command.
 * If arguments are passed to the command then 
 * "printenv | grep <arguments> | sort | pager" is be executed. The pager 
 * executed is selected primarily based on the value of the users "PAGER" 
 * environment variable. If no such variable is set then it tries to execute 
 * "less" and if that fails "more". 
 */
void handleCheckEnvCmd(const int cmdArgc, char *cmdArgv[]);

#endif
