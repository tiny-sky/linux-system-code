#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
//open函数
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFLEN 128

//服务器提供数据服务
void serve(int socket){

    int ConnerFD;
    int  fp;
    char buf[BUFLEN];

    for (;;){
        //接受套接字连接
        if(ConnerFD = accept(socket,NULL,NULL)<0){
            sprintf(STDERR_FILENO, "error:%s\n", strerror(errno));
            exit(1);
        }
        //读取数据
    if((fp = open("~/Work/Computer_theory/skill/skill.md","r") == NULL)){
            sprintf(STDERR_FILENO, "error:%s\n", strerror(errno));
            send(ConnerFD, buf, strlen(buf), 0);
    }else{
        while(fgets(buf,BUFLEN,fp) != NULL)
            send(ConnerFD, buf, strlen(buf), 0);
    }
    //关闭当前套接字连接
    close(ConnerFD);
    }
}