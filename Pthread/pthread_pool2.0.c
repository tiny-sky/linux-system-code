#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "pthread_pool2.0.h"

ThreadPool *threadPoolInit(int min, int max, int queueSize)
{
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    do
    {
        if (pool == NULL)
        {
            printf("malloc threadpool failed\n");
            break;
        }

        // 初始线程池参数
        pool->threadIDs = (ThreadPool *)malloc(sizeof(pthread_t) * max);
        if (pool->threadIDs == NULL)
        {
            printf("malloc threads failed\n");
            break;
        }
        memset(pool->threadIDs, 0, sizeof(pthread_t) * max); // 方便判断空余空间
        pool->maxnum = max;
        pool->minnum = min;
        pool->livenum = min;
        pool->busynum = 0;
        pool->exitnum = 0;

        if (pthread_mutex_init(&pool->mutexpool, NULL) != 0 ||
            pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
            pthread_cond_init(&pool->notEmpty, NULL) != 0 ||
            pthread_cond_init(&pool->notFull, NULL) != 0)
        {
            printf("mutex or condition init fail...\n");
            break;
        }

        // 任务队列
        pool->queueCapacity = queueSize;
        pool->queueSize = 0;
        pool->head = NULL;
        pool->tail = NULL;

        pool->shutdown = 0; // 当前为打开状态

        // 创建线程
        pthread_create(&pool->managerID, NULL, manager, (void *)pool);
        for (int i = 0; i < min; i++)
        {
            pthread_create(&pool->threadIDs[i], NULL, worker, (void *)pool);
        }
        return pool;
    } while (0);

    // 释放资源
    if (pool && pool->threadIDs)
        free(pool->threadIDs);
    if (pool)
        free(pool);

    return NULL;
}

void *worker(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;

    while (1)
    {
        phread_mutex_lock(&pool->mutexpool);
        // 判断任务队列是否为空
        while (pool->queueSize == 0 && !pool->shutdown)
        {
            // 阻塞线程
            pthread_cond_wait(&pool->notEmpty, &pool->mutexpool);

            // 判断此时线程池是否被关闭了
            if (pool->shutdown)
            {
                pthread_mutex_unlock(&pool->mutexpool);
                threadExit(pool);
            }
            // 从任务队列中取出一个任务，开始工作！
            Task task;
            task.function = pool->head->function;
            task.arg = pool->head->arg;
            task.next = NULL;
        }
        pthread_mutex_unlock(&pool->mutexpool);
    }
}