#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <unistd.h>

#include <fcntl.h>
#include <sys/wait.h>

using namespace std;

#define BUFLEN 128
#define QLEN 10

void serve (int sockfd){

    int Connerfd,status;
    FILE *fp;
    pid_t pid;

    for (;;){
        //被动套接字接受连续，产生通信套接字
        if((Connerfd = accept (sockfd,NULL,NULL)) < 0){
            cerr << "error:" << strerror(errno) << endl;
            exit(1);
        }
        //生成子进程开始执行服务器工作
        if((pid = fork() )< 0){
            cerr << "error:" << strerror(errno) << endl;
            exit(1);
        }else if(pid == 0){
            /*
             *通过创建的子进程来执行一个终端命令
             * 并通过重定向标准输入与输出来将数据传递到客户端            
             */
          if( dup2(Connerfd, STDERR_FILENO) <0||
            dup2(Connerfd, STDOUT_FILENO) <0){
              cerr << "error:" << strerror(errno) << endl;
              exit(1);
          }

          close(Connerfd);
          execl("/usr/bin/uptime", "uptime", NULL);
          cerr << "error:" << strerror(errno) << endl;
        }else { //父进程
          close(Connerfd);
          waitpid(pid, &status, 0);
        }
    }
}

int initserver(int type, struct sockaddr * addr, socklen_t len, int qlen){

    int fd,err=0;
    
    //产生被动套接字
    if((fd = socket (addr->sa_family,type,0)) < 0){
        return -1;
    }
    //将套接字与地址相关联
    if(bind(fd,addr,len) <0){
        goto errout;
    }
    //开始监听请求连接数据
    if(listen(fd,qlen) <0 ){
        goto errout;
    }
    return fd;
    
    //错误处理
    errout :
        err = errno;
        close(fd);
        errno = err;
        return -1;
    }

int main(){

    struct addrinfo *ailist ,*aip;
    struct addrinfo hint;
    int sockfd,err, n;
    char *host;
    
    //获取主机地址
    if((n = sysconf(_SC_HOST_NAME_MAX)) < 0){
        n = 1024;
    }
    if((host = (char *)malloc(n)) == NULL){
        cerr << "error:" << strerror(errno) << endl;
        exit(1);
    }
    if(gethostname(host,n)<0){
        cerr << "error:" << strerror(errno) << endl;
        exit(1);
    }

    //获取地址模板
    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_CANONIDN;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;
    if((err = getaddrinfo(host,"ruptime",&hint,&ailist)) != 0){
        cerr << "getaddrinfo error : " << strerror(err) << endl;
        exit(1);
    }

    //依次遍历符合要求的网络地址，提供服务
    for (aip = ailist; aip != NULL;aip = aip->ai_next){

        //初始化被动套接字
        if((sockfd = initserver(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen, QLEN)) >= 0){
            //被动套接字开始工作
          serve(sockfd);
          exit(0);
        }
    }
    freeaddrinfo(ailist);
    return 0;
}