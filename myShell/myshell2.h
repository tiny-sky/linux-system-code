#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define LEN 100
#define MAXCMD 20
#define MAXPATH 64

typedef struct Node{
    char *arglist[MAXCMD+1];
    int top;
}stack;

typedef struct node{
    int sign;
    stack stack;
}vertex;

char curdir[MAXPATH];
char olddir[MAXPATH];
char curdir2[MAXPATH];
char buf[1024];
int dirsign = 1;

void command_split (char *arglist);
void execute (vertex *);
void execute_exe (vertex *L);
void execute_pipe(vertex *L);
void execute_redir_r (vertex *L);
void execute_redir_a (vertex *L);
void execute_redir_B (vertex *L);
void execute_cmd (vertex *L);
void sys_err (char *errno,int num);
char *GetPATH(char *temp);
void shiSign(void);
void execute_cddir (char *PATH);