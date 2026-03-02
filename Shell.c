/**
 * File: Shell.c
 * Description: The main entry point for the custom Unix shell.
 * * CS 452 Implementation Details:
 * - Implements the main REPL (Read, Parse, Interpret) loop.
 * - Handles zombie process prevention by calling waitpid(..., WNOHANG) 
 * before each prompt to reap completed background jobs.
 * - Ensures zero memory leaks by cleanly freeing the job list and terminal 
 * state upon exiting.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "Jobs.h"
#include "Parser.h"
#include "Interpreter.h"
#include "error.h"

int main() {
  int eof=0;
  Jobs jobs=newJobs();
  char *prompt=0;

  if (isatty(fileno(stdin))) {
    using_history();
    read_history(".history");
    prompt="$ ";
  } else {
    rl_bind_key('\t',rl_insert);
    rl_outstream=fopen("/dev/null","w");
  }
  
  while (!eof) {
    /* Reap any background processes that have finished to prevent zombies */
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);

    char *line=readline(prompt);
    if (!line)
      break;
    if (*line)
      add_history(line);
    Tree tree=parseTree(line);
    free(line);
    interpretTree(tree,&eof,jobs);
    freeTree(tree);
  }

  if (isatty(fileno(stdin))) {
    write_history(".history");
    rl_clear_history();
  } else {
    fclose(rl_outstream);
  }
  freestateCommand();
  freeJobs(jobs);
  return 0;
}