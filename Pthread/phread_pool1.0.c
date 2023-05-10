#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

typedef struct job
{
    void (*function)(void *arg);
    void *arg;
    struct job *next;
} job;

typedef struct thread_pool
{
    int num_threads;
    pthread_t *threads;
    job *queue_head;
    job *queue_tail;
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_not_empty;
    int is_shutdown;
} thread_pool;

void *thread_function(void *arg);

// 初始化线程池
void thread_pool_init(thread_pool *pool, int num_threads)
{
    pool->num_threads = num_threads;
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    pool->queue_head = NULL;
    pool->queue_tail = NULL;
    pthread_mutex_init(&(pool->queue_lock), NULL);
    pthread_cond_init(&(pool->queue_not_empty), NULL);
    pool->is_shutdown = 0;

    // 创建线程
    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&(pool->threads[i]), NULL, thread_function, (void *)pool);
    }
}

// 销毁线程池
void thread_pool_destroy(thread_pool *pool)
{
    // 设置标志位，通知线程退出
    pool->is_shutdown = 1;

    // 等待所有线程退出
    pthread_cond_broadcast(&pool->queue_not_empty);

    for (int i = 0; i < pool->num_threads; i++)
    {
        pthread_join(pool->threads[i], NULL);
    }

    // 销毁互斥锁和条件变量
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_not_empty));

    // 释放内存
    free(pool->threads);
}

// 向线程池中添加任务
void thread_pool_enqueue(thread_pool *pool, void (*function)(void *arg), void *arg)
{
    job *new_job = (job *)malloc(sizeof(job));
    new_job->function = function;
    new_job->arg = arg;
    new_job->next = NULL;

    // 加锁修改队列
    pthread_mutex_lock(&(pool->queue_lock));

    if (pool->queue_tail == NULL)
    {
        pool->queue_head = new_job;
        pool->queue_tail = new_job;
    }
    else
    {
        pool->queue_tail->next = new_job;
        pool->queue_tail = new_job;
    }

    // 通知线程有新的任务可以执行
    pthread_cond_signal(&(pool->queue_not_empty));

    // 解锁
    pthread_mutex_unlock(&(pool->queue_lock));
}

// 线程函数
void *thread_function(void *arg)
{
    thread_pool *pool = (thread_pool *)arg;

    while (1)
    {
        // 加锁获取任务
        pthread_mutex_lock(&(pool->queue_lock));

        // 判断是否要退出
        while (pool->queue_head == NULL && !pool->is_shutdown)
        {
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->queue_lock));
        }

        if (pool->is_shutdown)
        {
            // 释放锁并退出线程
            pthread_mutex_unlock(&(pool->queue_lock));
            pthread_exit(NULL);
        }

        // 取出一个任务
        job *next_job = pool->queue_head;
        pool->queue_head = next_job->next;

        if (pool->queue_head == NULL)
        {
            pool->queue_tail = NULL;
        }

        // 释放锁
        pthread_mutex_unlock(&(pool->queue_lock));

        // 执行任务
        (next_job->function)(next_job->arg);
        free(next_job);
    }

    return NULL;
}
// 定义任务函数
void print_hello(void *arg)
{
    printf("Hello, %s!\n", (char *)arg);
}

int main()
{
    // 创建线程池
    thread_pool pool;
    thread_pool_init(&pool, 4);

    // 添加任务
    thread_pool_enqueue(&pool, print_hello, "Alice");
    thread_pool_enqueue(&pool, print_hello, "Bob");
    thread_pool_enqueue(&pool, print_hello, "Charlie");
    //thread_pool_enqueue(&pool, print_hello, "Charlie");

    // 等待所有任务完成并销毁线程池
    while (pool.queue_head != NULL)
    {
        // 等待队列为空
        sleep(1);
    }
    printf("222\n");
    thread_pool_destroy(&pool);

    return 0;
}