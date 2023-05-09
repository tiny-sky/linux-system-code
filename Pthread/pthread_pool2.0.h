#ifndef _PTHREADPOOL_H
#define _PTHREADPOOL_H

#include <pthread.h>

typedef struct ThreadPool ThreadPool;

// 任务结构体
typedef struct Task
{
    void (*function)(void *arg);
    void *arg;
    struct Task *next;
} Task;

// 线程池的定义
typedef struct ThreadPool
{
    // 任务队列
    int queueCapacity; // 容量
    int queueSize;     // 当前的任务个数
    Task *head;        // 任务队列的头指针
    Task *tail;        // 任务队列的尾指针

    // 线程
    pthread_t managerID;  // 管理者线程ID
    pthread_t *threadIDs; // 工作的线程ID
    int maxnum;           // 最大的线程数
    int minnum;           // 最小的线程数
    int busynum;          // 正在忙的线程数
    int livenum;          // 幸存的线程数
    int exitnum;          // 要销毁的线程数

    // 互斥量与条件变量
    pthread_mutex_t mutexpool; // 锁整个线程池
    pthread_mutex_t mutexBusy; // 锁住busynum变量
    pthread_cond_t notFull;    // 任务队列是否为满
    pthread_cond_t notEmpty;   // 任务队列是否为空

    int shutdown; // 是否消耗线程池
} ThreadPool;
// 线程的初始化
ThreadPool *threadPoolInit(int min, int max, int queueSize);
// 销毁线程池
int threadPoolDestroy (ThreadPool *Pool);
// 给线程池添加任务
void threadPoolAdd(ThreadPool *pool, void *(function)(void *), void *arg);
// 工作线程
void *worker(void *arg);
// 管理者线程
void *manager(void *arg);
// 回收当前线程，方便下次使用
void threadExit(ThreadPool *ThreadPool);
#endif