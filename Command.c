#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/history.h>

#include "Command.h"
#include "error.h"

typedef struct {
  char *file;
  char **argv;
  char *in_file;
  char *out_file;
} *CommandRep;

#define BIARGS CommandRep r, int *eof, Jobs jobs
#define BINAME(name) bi_##name
#define BIDEFN(name) static void BINAME(name) (BIARGS)
#define BIENTRY(name) {#name,BINAME(name)}

static char *owd=0;
static char *cwd=0;

static void builtin_args(CommandRep r, int n) {
  char **argv=r->argv;
  for (n++; *argv++; n--);
  if (n) ERROR("wrong number of arguments to builtin command");
}

BIDEFN(exit) {
  builtin_args(r,0);
  *eof=1;
  /* Wait for any running background jobs to finish before exiting */
  while (wait(NULL) > 0);
}

BIDEFN(pwd) {
  builtin_args(r,0);
  char buf[1024];
  if (getcwd(buf, sizeof(buf))) {
      printf("%s\n", buf);
      if(cwd) free(cwd);
      cwd = strdup(buf);
  }
}

BIDEFN(cd) {
  builtin_args(r,1);
  if (strcmp(r->argv[1],"-")==0) {
    if (!owd) ERROR("cd: OLDPWD not set");
    char *twd=cwd;
    cwd=owd;
    owd=twd;
    chdir(cwd);
    printf("%s\n", cwd);
  } else {
    if (chdir(r->argv[1]) != 0) {
      perror("chdir failed");
    } else {
      if (owd) free(owd);
      owd=cwd;
      char buf[1024];
      cwd=strdup(getcwd(buf, sizeof(buf)));
    }
  }
}

BIDEFN(history) {
    builtin_args(r,0);
    HIST_ENTRY **the_list = history_list();
    if (the_list) {
        for (int i = 0; the_list[i]; i++) {
            printf("%5d  %s\n", i + history_base, the_list[i]->line);
        }
    }
}

static int builtin(BIARGS) {
  typedef struct {
    char *s;
    void (*f)(BIARGS);
  } Builtin;
  static const Builtin builtins[]={
    BIENTRY(exit),
    BIENTRY(pwd),
    BIENTRY(cd),
    BIENTRY(history),
    {0,0}
  };
  int i;
  for (i=0; builtins[i].s; i++)
    if (!strcmp(r->file,builtins[i].s)) {
      builtins[i].f(r,eof,jobs);
      return 1;
    }
  return 0;
}

static char **getargs(T_words words) {
  int n=0; T_words p=words;
  while (p) { p=p->words; n++; }
  char **argv=(char **)malloc(sizeof(char *)*(n+1));
  if (!argv) ERROR("malloc() failed");
  p=words; int i=0;
  while (p) { argv[i++]=strdup(p->word->s); p=p->words; }
  argv[i]=0;
  return argv;
}

extern Command newCommand(T_words words, char *in_file, char *out_file) {
  CommandRep r=(CommandRep)malloc(sizeof(*r));
  if (!r) ERROR("malloc() failed");
  r->argv=getargs(words);
  r->file=r->argv[0];
  r->in_file = in_file ? strdup(in_file) : 0;
  r->out_file = out_file ? strdup(out_file) : 0;
  return r;
}

static void child(CommandRep r, int fg, int fd_in, int fd_out) {
  /* Handle pipeline redirections */
  if (fd_in != 0) { dup2(fd_in, 0); close(fd_in); }
  if (fd_out != 1) { dup2(fd_out, 1); close(fd_out); }

  /* Handle specific file redirections (< and >) overrides pipelines */
  if (r->in_file) {
      int fd = open(r->in_file, O_RDONLY);
      if (fd < 0) { perror("open in_file failed"); exit(1); }
      dup2(fd, 0); close(fd);
  }
  if (r->out_file) {
      int fd = open(r->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd < 0) { perror("open out_file failed"); exit(1); }
      dup2(fd, 1); close(fd);
  }

  execvp(r->argv[0],r->argv);
  perror("execvp() failed");
  exit(1);
}

extern int execCommand(Command command, Pipeline pipeline, Jobs jobs,
			int *jobbed, int *eof, int fg, int fd_in, int fd_out) {
  CommandRep r=command;
  if (fg && builtin(r,eof,jobs)) return 0;
  
  if (!*jobbed) {
    *jobbed=1;
    addJobs(jobs,pipeline);
  }

  int pid=fork();
  if (pid==-1) ERROR("fork() failed");
  if (pid==0) child(r,fg,fd_in,fd_out);
  return pid;
}

extern void freeCommand(Command command) {
  CommandRep r=command;
  char **argv=r->argv;
  while (*argv) free(*argv++);
  free(r->argv);
  if (r->in_file) free(r->in_file);
  if (r->out_file) free(r->out_file);
  free(r);
}

extern void freestateCommand() {
  if (cwd) free(cwd);
  if (owd) free(owd);
}