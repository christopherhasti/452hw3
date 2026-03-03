#ifndef COMMAND_H
#define COMMAND_H

typedef void *Command;

#include "Tree.h"
#include "Jobs.h"
#include "Sequence.h"

/* Updated to accept in_file and out_file */
extern Command newCommand(T_words words, char *in_file, char *out_file);

/* Updated to accept fd_in and fd_out for pipelines/redirection, and returns an int (pid) */
extern int execCommand(Command command, Pipeline pipeline, Jobs jobs,
			int *jobbed, int *eof, int fg, int fd_in, int fd_out);

extern void freeCommand(Command command);
extern void freestateCommand();

#endif