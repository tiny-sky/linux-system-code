#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>

#include <fcntl.h>
#include <sys/wait.h>

#define BUFLEN 128
#define QLEN 10

using namespace std;

void serve(int sockfd)
{

    int Connerfd, status;
    FILE *fp;
    pid_t pid;

    for (;;)
    {
        cout << "开始server工作" << endl;
        // 被动套接字接受连续，产生通信套接字
        if ((Connerfd = accept(sockfd, NULL, NULL)) < 0)
        {
            std::cerr << "error:" << strerror(errno) << std::endl;
            exit(1);
        }
        cout << "通信套接字获取成功：" << Connerfd << endl;
        // 生成子进程开始执行服务器工作
        if ((pid = fork()) < 0)
        {
            std::cerr << "error:" << strerror(errno) << std::endl;
            exit(1);
        }
        else if (pid == 0)
        {
            /*
             *通过创建的子进程来执行一个终端命令
             * 并通过重定向标准输入与输出来将数据传递到客户端
             */
            if (dup2(Connerfd, STDERR_FILENO) < 0 ||
                dup2(Connerfd, STDOUT_FILENO) < 0)
            {
                std::cerr << "error:" << strerror(errno) << std::endl;
                exit(1);
            }

            close(Connerfd);
            execl("/usr/bin/uptime", "uptime", NULL);
            std::cerr << "error:" << strerror(errno) << std::endl;
        }
        else
        { // 父进程
            close(Connerfd);
            waitpid(pid, &status, 0);
        }
    }
}

int initserver(int type, struct sockaddr *addr, socklen_t len, int qlen)
{

    int fd, err = 0;

    // 产生被动套接字
    if ((fd = socket(addr->sa_family, type, 0)) < 0)
    {
        return -1;
    }
    cout << "被动socket " << fd << "初始化完成......" << endl;
    // 将套接字与地址相关联
    if (bind(fd, addr, len) < 0)
    {
        goto errout;
    }
    cout << "bind " << fd << "关联完成......" << endl;
    // 开始监听请求连接数据
    cout << "开始监听......" << endl;
    if (listen(fd, qlen) < 0)
    {
        goto errout;
    }
    return fd;

// 错误处理
errout:
    cout << "监听失败......" << endl;
    err = errno;
    close(fd);
    errno = err;
    return -1;
}

int main()
{

    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int sockfd, err, n;
    char *host;

    // 获取主机地址
    if ((n = sysconf(_SC_HOST_NAME_MAX)) < 0)
    {
        n = 1024;
    }
    if ((host = (char *)malloc(n)) == NULL)
    {
        std::cerr << "error:" << strerror(errno) << std::endl;
        exit(1);
    }
    if (gethostname(host, n) < 0)
    {
        std::cerr << "error:" << strerror(errno) << std::endl;
        exit(1);
    }
    cout << "主机名获取成功：" << host << endl;
    // 获取地址模板
    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_CANONIDN;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;
    if ((err = getaddrinfo(host, "80", &hint, &ailist)) != 0)
    {
        std::cerr << "getaddrinfo error : " << strerror(err) << std::endl;
        exit(1);
    }

    // 依次遍历符合要求的网络地址，提供服务
    for (aip = ailist; aip != NULL; aip = aip->ai_next)
    {

        // 初始化被动套接字
        if ((sockfd = initserver(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen, QLEN)) >= 0)
        {
            cout << "初始化被动套接字完成：" << sockfd << endl;
            // 被动套接字开始工作
            serve(sockfd);
            exit(0);
        }
    }
    freeaddrinfo(ailist);
    return 0;
}