/**
 * File: Interpreter.c
 * Description: Traverses the AST and queues commands for execution.
 * * CS 452 Implementation Details:
 * - Passes I/O redirection target files from the Tree nodes down into 
 * the Command creation module.
 * - Inspects Sequence operators to determine execution mode. If the 
 * operator is '&', the pipeline is flagged to run in the background (fg=0). 
 * If ';', it runs in the foreground (fg=1).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Interpreter.h"
#include "Sequence.h"
#include "Pipeline.h"
#include "Command.h"

static Command i_command(T_command t);
static void i_pipeline(T_pipeline t, Pipeline pipeline);
static void i_sequence(T_sequence t, Sequence sequence);

static Command i_command(T_command t) {
  if (!t)
    return 0;
  Command command=0;
  if (t->words)
    /* Updated to pass in_file and out_file from the tree node */
    command=newCommand(t->words, t->in_file, t->out_file);
  return command;
}

static void i_pipeline(T_pipeline t, Pipeline pipeline) {
  if (!t)
    return;
  addPipeline(pipeline,i_command(t->command));
  i_pipeline(t->pipeline,pipeline);
}

static void i_sequence(T_sequence t, Sequence sequence) {
  if (!t)
    return;
    
  /* Check if it's a background or foreground job */
  int fg = 1;
  if (t->op && strcmp(t->op, "&") == 0) {
      fg = 0;
  }
  
  Pipeline pipeline=newPipeline(fg);
  i_pipeline(t->pipeline,pipeline);
  addSequence(sequence,pipeline);
  i_sequence(t->sequence,sequence);
}

extern void interpretTree(Tree t, int *eof, Jobs jobs) {
  if (!t)
    return;
  Sequence sequence=newSequence();
  i_sequence(t,sequence);
  execSequence(sequence,jobs,eof);
}