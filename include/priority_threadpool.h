#ifndef __PRIORITY_THREADPOOL_H__
#define __PRIORITY_THREADPOOL_H__

#include <pthread.h>

#define NUM_PRIORITY_LEVELS  10    // 优先级从 1 到 10
#define MAX_WORKERS          20    // 最多 20 个工作线程

// 任务结构体
typedef struct nTask {
    void (*task_func)(struct nTask *);
    void *user_data;
    int priority;                  // 优先级 1-10，数字越大优先级越高
    
    struct nTask *prev;
    struct nTask *next;
} Task;

// 工作线程结构体
struct nWorker {
    pthread_t threadid;
    int terminate;                 // 1: 下班  0: 工作
    struct PriorityThreadPool *manager;
    
    struct nWorker *prev;
    struct nWorker *next;
};

// 优先级线程池结构体
typedef struct PriorityThreadPool {
    struct nWorker *workers;                          // 工作线程链表
    struct nTask *tasks[NUM_PRIORITY_LEVELS];        // 按优先级分类的任务队列
    
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    
    int num_workers;
    int shutdown;
} PriorityThreadPool;

// API 接口
int nPriorityThreadPoolCreate(PriorityThreadPool *pool, int numWorkers);
int nPriorityThreadPoolPushTask(PriorityThreadPool *pool, struct nTask *task);
int nPriorityThreadPoolDestroy(PriorityThreadPool *pool);

#endif
