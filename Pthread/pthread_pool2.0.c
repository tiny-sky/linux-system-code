#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
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
            printf("mutex or condition init failed...\n");
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

void threadPoolAdd(ThreadPool *pool, void *(function)(void *), void *arg)
{
    pthread_mutex_lock(&pool->mutexpool);
    // 判断线程池是否关闭以及是否还有空域位置
    while (!pool->shutdown && pool->queueSize == pool->queueCapacity)
    {
        pthread_cond_wait(&pool->notFull, &pool->mutexpool);
    }
    // 可能得到锁之后线程池关闭
    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutexpool);
        return;
    }
    // 添加任务
    Task *job = (Task *)malloc(sizeof(Task));
    if (job == NULL)
    {
        pthread_mutex_unlock(&pool->mutexpool);
        return -1;
    }
    job->function = function;
    job->arg = arg;
    job->next = NULL;
    if (pool->head == NULL)
    {
        pool->head = pool->tail = job;
        pthread_cond_signal(&pool->notEmpty);
    }
    else
    {
        pool->tail->next = job;
        pool->tail = job;
    }
    pool->livenum++;
    pthread_mutex_unlock(&pool->mutexpool);

    return 0;
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
            // 判断是否有要销毁的线程
            if (pool->exitnum > 0)
            {
                pool->exitnum--;
                if (pool->livenum > pool->minnum)
                {
                    pool->livenum--;
                    pthread_mutex_unlock(&pool->mutexpool);
                    threadExit(pool);
                }
            }
        }
        // 判断此时线程池是否被关闭了
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexpool);
            threadExit(pool);
        }
        // 从任务队列中取出一个任务
        Task task;
        task.function = pool->head->function;
        task.arg = pool->head->arg;
        task.next = NULL;
        // 移动头节点
        pool->head = pool->head->next;
        pool->queueSize--;
        // 取出线程，唤醒生产者线程
        pthread_cond_signal(&pool->notFull);
        pthread_mutex_unlock(&pool->mutexpool);

        // 正式开始工作
        printf("thread %ld start working...\n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busynum++;
        pthread_mutex_unlock(&pool->mutexBusy);
        task.function(task.arg);
        free(task.arg);
        task.arg = NULL;

        // 工作结束
        printf("thread %ld end working...\n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busynum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    return NULL;
}

void *manager(void *arg)
{
    sleep(3);
    ThreadPool *pool = (ThreadPool *)arg;

    while (!pool->shutdown)
    {
        // 取出线程池中任务数量与线程数量
        pthread_mutex_lock(&pool->mutexpool);
        int queuenum = pool->queueSize;
        int livenum = pool->livenum;
        pthread_mutex_unlock(&pool->mutexpool);

        // 取出正在忙的线程数
        pthread_mutex_lock(&pool->mutexBusy);
        int busynum = pool->busynum;
        pthread_mutex_unlock(&pool->mutexBusy);

        // 添加线程
        // 条件：任务个数>现存线程个数 && 现存线程个数<最大线程数
        if (queuenum > livenum && livenum < pool->maxnum)
        {
            pthread_mutex_lock(&pool->mutexpool);
            for (int i = 0; i + livenum < queuenum && i + livenum < pool->maxnum; i++)
            {
                if (pool->threadIDs[i] == 0)
                {
                    pthread_create(pool->threadIDs[i], NULL, worker, pool);
                    pool->livenum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexpool);
        }

        // 销毁线程
        // 条件： 任务个数 *2 < 现存线程个数 &&现存线程个数 > 最小线程数
        if (queuenum * 2 < livenum && livenum > pool->minnum)
        {
            pthread_mutex_lock(&pool->mutexpool);
            pool->exitnum = 2;
            pthread_mutex_unlock(&pool->mutexpool);
            // 让线程自杀
            for (int i = 0; i < pool->exitnum; i++)
            {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }
    return NULL;
}

int threadPoolDestroy(ThreadPool *pool)
{
    if (pool == NULL)
        return -1;

    // 关闭线程池
    pool->shutdown = 1;
    // 回收管理者线程
    pthread_join(pool->managerID, NULL);
    // 先唤醒任务线程，然后再回收
    pthread_cond_broadcast(&pool->notEmpty);
    for (int i = 0; i < pool->maxnum; i++)
    {
        if (pool->threadIDs[i] != 0)
        {
            pthread_join(&pool->threadIDs[i], NULL);
        }
    }
    // 释放堆内存
    struct Task *temp;
    while (pool->head != NULL)
    {
        temp = pool->head;
        pool->head = temp->next;
        free(temp);
    }
    if (pool->threadIDs)
    {
        free(pool->threadIDs);
    }
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_mutex_destroy(&pool->mutexpool);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    free(pool);
    pool = NULL;

    return 0;
}

void threadExit(ThreadPool *pool)
{
    pthread_t tid = pthread_self();
    for (int i = 0; i < pool->maxnum; i++)
    {
        if (pool->threadIDs[i] == tid)
        {
            pool->threadIDs[i] = 0;
            printf("thread %ld exiting...\n", tid);
            break;
        }
    }
    pthread_exit(NULL);
}