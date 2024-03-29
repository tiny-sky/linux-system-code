#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>

using namespace std;

#define BUFLEN 1024
#define MAXSLEEP 128

void print_uptime (int sockfd){

    int n;
    char buf[BUFLEN];

    //执行读取操作

    if((n = recv(sockfd,buf,BUFLEN,0)) >0){                                 //这样处理是有问题的，当读取的数据大于BUFLEN时，剩余的将不会读取

        cout << "(服务端)sockfd:" << sockfd << "发送数据：" << '\t' << flush;
        if(write(STDOUT_FILENO, buf, n)<0){
            cerr << strerror(errno) << endl;
            exit(1);
        }
    }
  
    if(n < 0){
        cerr << "error:" << strerror(errno) << endl;
        exit(1);
    }
}

int connect_retry (int domain,int type,int protocol,const struct sockaddr* addr,socklen_t alen){

    int numsec,fd;

    for (numsec = 1; numsec <= MAXSLEEP; numsec = numsec << 1){
        //生成套接字
      if( (fd = socket(domain,type,protocol)) <0)
           return -1;

       // 连接服务器
       if (connect(fd, addr, alen) == 0){
           return fd;
       }
        close(fd);

        //设置睡眠时长，并多次尝试
        if(numsec <= MAXSLEEP/2){
           cout << "连接失败......当前睡眠：" << numsec << endl;
           sleep(numsec);
        }
    }

    return -1;
}

int main(){

    struct addrinfo *alist, *aip;
    struct addrinfo hint;
    int sockfd, err,n;
    char *host;
    string buf(BUFLEN, '\0');

    //读取sockaddr
    string ip_str(INET_ADDRSTRLEN,'\0');
    string host_str(NI_MAXHOST, '\0');
    string service_str(NI_MAXSERV, '\0');

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
    hint.ai_family = AF_INET;
    hint.ai_flags = AI_CANONIDN;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(host, "9000", &hint, &alist)) != 0)
    {
        cerr << "getaddrinfo error : " << strerror(err) << endl;
        exit(1);
    }

    // 依次遍历符合要求的网络地址，发送连接需求
    for (aip = alist; aip != NULL; aip = aip->ai_next){
        
        //显示获取的地址
        inet_ntop(AF_INET, (struct sockaddr*)&(aip->ai_addr), &ip_str[0], ip_str.size());
        if((err = getnameinfo((struct sockaddr *)aip->ai_addr, aip->ai_addrlen, &host_str[0], NI_MAXHOST, &service_str[0], NI_MAXSERV,NI_NUMERICSERV))!=0){
           cerr << "getnameinfo error:" << gai_strerror(err) << endl;
           exit(1);
        }
        cout << "************Host information*************" << endl;
        cout << '\t'<<"IP address :" << ip_str<< endl;
        cout <<'\t'<< "Host :" << host_str << endl;
        cout <<'\t'<< "Service :" << service_str<< endl;
        cout << "***************************************" << endl;

        //尝试多次连接 
        if((sockfd= connect_retry(aip->ai_family,SOCK_STREAM,0,aip->ai_addr,aip->ai_addrlen)) <0){
           cerr << "error:" << strerror(errno) << endl;
        }else{
           cout << "当前连接成功sockfd:" << sockfd << '\n' << endl;
           while(1){

               cout << "请向服务端" << sockfd << "发送数据："<<flush;
               fgets(&buf[0], BUFLEN, stdin);
               if(strncmp(&buf[0],"exit",4) ==0 ){
                   break;
               }    
                if(send(sockfd, &buf[0], strlen(&buf[0]), 0)<0){
                   cerr << "send error:" << strerror(errno) << endl;
                   exit(1);
                }
               print_uptime(sockfd);
           }
           close(sockfd);
           return 0;
        }
    }
}

 