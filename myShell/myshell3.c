#include "myshell2.h"

vertex L[8];

int main()
{
    char argbuf[LEN]; 

    while(1)
    {
        getcwd(curdir,MAXPATH);
        shiSign();
        printf("myshell >\033[4m%s\033[0m $:",curdir);
        if(fgets(argbuf,LEN,stdin)&&*argbuf!='\n')
         {
            command_split(argbuf);
            execute(L);
         }   
    }
}

void sys_err (char *errno,int num){
    perror(errno);
    exit(num);
}

char *GetPATH(char *temp){
    char *PATH=getenv("HOME");
    return temp=strcat(PATH,temp+1);;
}

void shiSign(){
    signal(SIGINT,SIG_IGN);
    signal(SIGHUP,SIG_IGN);
}

void command_split (char *arglist)
{
    int i=0;
    L[i].stack.top=-1;
    L[i].sign=0;
    arglist[strlen(arglist)-1]='\0';

    char *temp=strtok(arglist, " ");
    while(temp!=NULL)
    {
        if(*temp=='\''){
            int i = strlen(temp);
            temp[i++]=' ';
            while(temp[i]!='|')   
            i++;
            temp[i-1] = '\0';
        }
        if(*temp=='|'||*temp=='>'||*temp=='<'||*temp=='&'){
            if(strcmp(temp,"|")==0)  L[i].sign=1;
            if(strcmp(temp,">")==0)  L[i].sign=2;
            if(strcmp(temp,"<")==0)  L[i].sign=3;
            if(strcmp(temp,">>")==0) L[i].sign=4;
            if(strcmp(temp,"&")==0)  L[i].sign=5;
            L[i].stack.arglist[L[i].stack.top+1]=NULL;
            L[++i].stack.top=-1;
            L[i].sign=0;
        }
        else
        {
            if(*temp=='~')
            temp=GetPATH(temp);
            L[i].stack.arglist[++L[i].stack.top]=temp;
        }
        temp=strtok(NULL," ");
    }
    L[i].stack.arglist[L[i].stack.top+1]=NULL;
    return ;
}

void execute (vertex * L)
{
    if(L->sign==0 || L->sign==5)
        execute_cmd(L);
    if(L->sign==1 || L->sign==3)
        execute_pipe(L);
    if(L->sign==2)
        execute_redir_r(L);
    if(L->sign==4)
        execute_redir_a(L);
}

void execute_cddir (char *path)
{   

    if((dirsign++) == 1)
        getcwd(curdir2,MAXPATH);
    else{
        memmove(olddir,curdir2,strlen(curdir2));
        getcwd(curdir2,MAXPATH);
    }
    if(*path=='-')
        path=olddir; 
    if(chdir(path)<0)
        sys_err("chdir failed",3);
}

void execute_pipe (vertex *L)
{
    int i=0,pid,n,sign=1,fd;
    int pipes[8][2];

    if(L->sign==3){
        fd = open((L+1)->stack.arglist[0],O_RDWR);
        dup2(fd,0);
    }

    do
    {   
        if(L->sign!=1 || L->sign!=2)
            sign=0;

        pipe(pipes[i]);
        pid=fork();

        if(pid<0)
            sys_err("fork failed",1);

        if(pid==0){

            if(i>0){
                dup2(pipes[i-1][0],0);
                close(pipes[i-1][0]);
            }
            dup2(pipes[i][1],1);
            close(pipes[i][1]);
            execvp(L->stack.arglist[0],L->stack.arglist);
        }
       
       if(pid>0){
            if(L->sign==1)  L++;
            if(L->sign==2)  L+=2;
            close(pipes[i++][1]);
       }

    } while (L->sign==1 || sign==1);

    if(L->sign==0){

        while((n = read(pipes[--i][0],buf,sizeof(buf)))>0)
            write(STDOUT_FILENO,buf,n);

    }else if(L->sign==2){
            
        int fd = open((L+1)->stack.arglist[0],O_RDWR|O_CREAT|O_TRUNC,0644);
        if(fd<0)
            sys_err("open failed",0);
            
        while((n=read(pipes[--i][0],buf,sizeof(buf)))>0)
            write(fd,buf,n);

        close(fd);

    }else if(L->sign==4){

        int fd = open((L+1)->stack.arglist[0],O_RDWR|O_APPEND|O_CREAT,0644);
        if(fd<0)
            sys_err("open failed",0);

        while((n=read(pipes[--i][0],buf,sizeof(buf)))>0)
            write(fd,buf,n);
                
        close(fd);
    }
    if(L->sign==3){
       
        dup2(0,fd);
        close(fd);
    }
    
    for(int j=0;j<=i;j++)
        wait(NULL);
}

void execute_cmd (vertex *L)   
{
    if(memcmp("cd",L->stack.arglist[0],2)==0)
            execute_cddir(L->stack.arglist[1]);
    else if(memcmp("exit",L->stack.arglist[0],4)==0)
            exit(1);
    else 
        execute_exe(L);
}

void execute_exe (vertex *L)
{
    int pid;
    pid =fork();

    if(pid<0)
        sys_err("fork failed",1);
    if(pid==0)
        if(execvp(L->stack.arglist[0],L->stack.arglist)<0)
            sys_err("execvp failed",2);
    if(pid>0){
        if(L->sign==0)
            wait(NULL);
        else    
            waitpid(pid,NULL,WNOHANG);
    }
}

void execute_redir_r (vertex *L)
{

    int pid = fork();
    int fd = open((L+1)->stack.arglist[0],O_WRONLY|O_CREAT|O_TRUNC,0644);

    if(pid<0)
        sys_err("fork failed",1);

    if(pid==0){

        if(dup2(fd,1)==-1)
            sys_err("dup2 failed",0);

        execvp(L->stack.arglist[0],L->stack.arglist);
    }

    if(pid>0){                       

        wait(NULL);     
        close(fd);
    }
}

void execute_redir_a (vertex *L)
{
    int pid = fork();
    int fd = open((L+1)->stack.arglist[0],O_WRONLY|O_CREAT|O_APPEND,0644);

    if(pid<0)
        sys_err("fork failed",1);

    if(pid==0){

        if(dup2(fd,1)==-1)
            sys_err("dup2 failed",0);

        execvp(L->stack.arglist[0],L->stack.arglist);
    }

    if(pid>0){                       

        wait(NULL);     
        close(fd);
    }
}
