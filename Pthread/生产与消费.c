#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

//链表的节点
struct Node{
    int number;
    struct Node *next;
};

struct Node * head =NULL;   //定义头节点
pthread_mutex_t mutex;  //定义互斥锁
pthread_cond_t cond;    //定义互斥量

//生产者调用函数
void* producer(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        //创建链表(头插法)
        struct Node *newnode = (struct Node*)malloc(sizeof(struct Node));
        newnode->number = rand() % 1000;
        newnode->next = head;
        head = newnode;
        printf("生产者%ld:创建了数据%d\n",pthread_self(),newnode->number);
        pthread_mutex_unlock(&mutex);
        //唤醒消费者
        pthread_cond_broadcast(&cond);
        sleep(rand()%3);
    }
    return NULL;
}

//消费者调用函数
void *consumer(void *arg){
    while (1){
        pthread_mutex_lock(&mutex);
        //如何当前为空
        while(head == NULL){
            pthread_cond_wait(&cond,&mutex);
        }
        //调用头号任务
        struct Node *node = head;
        printf("消费者%ld:消耗了数据%d\n",pthread_self(),node->number);
        //消耗数据
        head = node->next;
        free(node);
        pthread_mutex_unlock(&mutex);

        sleep(rand()%3);
        
    }
    return NULL;
}

int main()
{
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);

    pthread_t t1[5],t2[5];
    for(int i = 0 ; i<5 ;i++){
        pthread_create(&t1[i],NULL,producer,NULL);
    }
     for(int i = 0 ; i<5 ;i++){
        pthread_create(&t2[i],NULL,consumer,NULL);
    }
     for(int i = 0 ; i<5 ;i++){
        pthread_join(&t1[i],NULL);
        pthread_join(&t2[i],NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}