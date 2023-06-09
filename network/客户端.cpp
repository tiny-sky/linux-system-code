#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <iostream>

using namespace std;

#define BUFLEN 128
#define MAXSLEEP 128

void print_uptime (int sockfd){

    int n;
    char buf[BUFLEN];

    //执行读取操作
    while((n = recv(sockfd,buf,BUFLEN,0 )) >0){
        write(STDOUT_FILENO, buf, n);
    }
    if(n < 0){
        cerr << "error:" << strerror(errno) << endl;
        exit(1);
    }
}

int connect_retry (int domain,int type,int protocol,const struct sockaddr* addr,socklen_t alen){

    int numsec,fd;

    for (numsec = 1; numsec <= MAXSLEEP;numsec << 1){
        //生成套接字
       if( (fd = socket(domain,type,protocol)) <0)
           return -1;
        //连接服务器
        if(connect(fd,addr,alen) ==0)
           return fd;

        close(fd);

        //设置睡眠时长，并多次尝试
        if(numsec <= MAXSLEEP/2)
           sleep(numsec);
    }

    return -1;
}

int main(){

    struct addrinfo *alist, *aip;
    struct addrinfo hint;
    int sockfd, err,n;
    char *host;

    // 获取主机地址
    if ((n = sysconf(_SC_HOST_NAME_MAX)) < 0)
    {
        n = 1024;
    }
    if ((host = (char *)malloc(n)) == NULL)
    {
        cerr << "error:" << strerror(errno) << endl;
        exit(1);
    }
    if (gethostname(host, n) < 0)
    {
        cerr << "error:" << strerror(errno) << endl;
        exit(1);
    }

    //获取地址模板
    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_CANONIDN;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;
    if ((err = getaddrinfo(host, "ruptime", &hint, &alist)) != 0)
    {
        cerr << "getaddrinfo error : " << strerror(err) << endl;
        exit(1);
    }

    // 依次遍历符合要求的网络地址，发送连接需求
    for (aip = alist; aip != NULL; aip = aip->ai_next){
        //尝试多次连接
        if((sockfd= connect_retry(aip->ai_family,SOCK_STREAM,0,aip->ai_addr,aip->ai_addrlen)) <0){
           cerr << "error:" << strerror(errno) << endl;
        }else{
           print_uptime(sockfd);
           exit(1);
        }
    }
}

 