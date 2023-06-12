#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>

using namespace std;

#define BUFLEN 128
#define MAXSLEEP 128

void print_uptime (int sockfd){

    int n;
    char buf[BUFLEN];

    //执行读取操作
    while((n = recv(sockfd,buf,BUFLEN,0 )) >0){
        cout << "当前读取数据字节为：" << n << endl;
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
       cout << "socket " << fd << "初始化完成......" << endl;

       // 连接服务器
       if (connect(fd, addr, alen) == 0){
           cout << "connect" << fd << "连接成功......" << endl;
           return fd;
       }
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

    //读取sockaddr
    string ip_str;

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
    cout << "主机名获取成功：" << host << endl;
    //获取地址模板
    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_CANONIDN;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(host, "80", &hint, &alist)) != 0)
    {
        cerr << "getaddrinfo error : " << strerror(err) << endl;
        exit(1);
    }

    // 依次遍历符合要求的网络地址，发送连接需求
    for (aip = alist; aip != NULL; aip = aip->ai_next){
        
        //显示获取的地址
        inet_ntop(AF_INET6, &(aip->ai_addr), ip_str.c_str, INET_ADDRSTRLEN);

        //尝试多次连接 
        if((sockfd= connect_retry(aip->ai_family,SOCK_STREAM,0,aip->ai_addr,aip->ai_addrlen)) <0){
           cerr << "error:" << strerror(errno) << endl;
        }else{
           cout << "当前连接成功sockfd:" << sockfd << endl;
           print_uptime(sockfd);
           exit(1);
        }
    }
}

 