#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Pipeline.h"
#include "deq.h"
#include "error.h"

typedef struct {
  Deq processes;
  int fg;
} *PipelineRep;

extern Pipeline newPipeline(int fg) {
  PipelineRep r=(PipelineRep)malloc(sizeof(*r));
  if (!r) ERROR("malloc() failed");
  r->processes=deq_new();
  r->fg=fg;
  return r;
}

extern void addPipeline(Pipeline pipeline, Command command) {
  PipelineRep r=(PipelineRep)pipeline;
  deq_tail_put(r->processes,command);
}

extern int sizePipeline(Pipeline pipeline) {
  PipelineRep r=(PipelineRep)pipeline;
  return deq_len(r->processes);
}

static void execute(Pipeline pipeline, Jobs jobs, int *jobbed, int *eof) {
  PipelineRep r=(PipelineRep)pipeline;
  int num_cmds = sizePipeline(pipeline);
  int pids[num_cmds];
  int fd_in = 0;
  int fd[2];

  for (int i = 0; i < num_cmds && !*eof; i++) {
    Command cmd = deq_head_ith(r->processes, i);
    
    if (i < num_cmds - 1) {
      pipe(fd);
      pids[i] = execCommand(cmd, pipeline, jobs, jobbed, eof, r->fg, fd_in, fd[1]);
      close(fd[1]);
      if (fd_in != 0) close(fd_in);
      fd_in = fd[0];
    } else {
      pids[i] = execCommand(cmd, pipeline, jobs, jobbed, eof, r->fg, fd_in, 1);
      if (fd_in != 0) close(fd_in);
    }
  }

  /* Wait for all pipeline children if this is a foreground pipeline */
  if (r->fg) {
    for (int i = 0; i < num_cmds; i++) {
      if (pids[i] > 0) {
        int status;
        waitpid(pids[i], &status, 0);
      }
    }
  }
}

extern void execPipeline(Pipeline pipeline, Jobs jobs, int *eof) {
  int jobbed=0;
  execute(pipeline,jobs,&jobbed,eof);
  if (!jobbed)
    freePipeline(pipeline);
}

extern void freePipeline(Pipeline pipeline) {
  PipelineRep r=(PipelineRep)pipeline;
  deq_del(r->processes,freeCommand);
  free(r);
}