#include "priority_threadpool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// 链表宏定义（头插法）
#define LIST_INSERT(item, list) do {\
    (item)->prev = NULL;\
    (item)->next = (list);\
    if ((list) != NULL) (list)->prev = (item);\
    (list) = (item);\
}while(0)

// 链表宏定义（移除节点）
#define LIST_REMOVE(item, list) do {\
    if ((item)->next != NULL) (item)->next->prev = (item)->prev;\
    if ((item)->prev != NULL) (item)->prev->next = (item)->next;\
    if ((item) == (list)) (list) = (item)->next;\
    (item)->next = NULL;\
    (item)->prev = NULL;\
}while(0)

// 工作线程回调函数
void *nThreadPoolCallback(void *arg) {
    struct nWorker *worker = (struct nWorker *)arg;

    while(1) {
        pthread_mutex_lock(&worker->manager->mutex);

        // 防御性编程：检查所有优先级队列是否都为空
        int has_task = 0;
        for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
            if (worker->manager->tasks[i] != NULL) {
                has_task = 1;
                break;
            }
        }

        // 如果没有任务，等待被唤醒
        while (!has_task) {
            if (worker->terminate) break;
            pthread_cond_wait(&worker->manager->cond, &worker->manager->mutex);

            // 再检查一遍
            has_task = 0;
            for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
                if (worker->manager->tasks[i] != NULL) {
                    has_task = 1;
                    break;
                }
            }
        }

        if (worker->terminate) {
            pthread_mutex_unlock(&worker->manager->mutex);
            break;
        }

        // ✅ 从最高优先级开始获取任务
        // NUM_PRIORITY_LEVELS - 1 对应优先级 10
        // 0 对应优先级 1
        struct nTask *task = NULL;
        for (int i = NUM_PRIORITY_LEVELS - 1; i >= 0; i--) {
            if (worker->manager->tasks[i] != NULL) {
                task = worker->manager->tasks[i];
                LIST_REMOVE(task, worker->manager->tasks[i]);   
                break;
            }
        }

        pthread_mutex_unlock(&worker->manager->mutex);

        // 执行任务
        if (task != NULL) {
            task->task_func(task);
        }
    }

    return NULL;
}

// 创建线程池
int nPriorityThreadPoolCreate(PriorityThreadPool *pool, int numWorkers) {
    if (pool == NULL) return -1;
    if (numWorkers < 1) numWorkers = 1;
    if (numWorkers > MAX_WORKERS) numWorkers = MAX_WORKERS;

    memset(pool, 0, sizeof(PriorityThreadPool));

    pthread_cond_init(&pool->cond, NULL);
    pthread_mutex_init(&pool->mutex, NULL);

    pool->num_workers = numWorkers;
    pool->shutdown = 0;

    // 创建工作线程
    for (int i = 0; i < numWorkers; i++) {
        struct nWorker *worker = (struct nWorker *)malloc(sizeof(struct nWorker));
        if (worker == NULL) {
            perror("malloc");
            return -2;
        }
        memset(worker, 0, sizeof(struct nWorker));

        worker->manager = pool;
        worker->terminate = 0;

        // 将工作线程加入链表
        LIST_INSERT(worker, pool->workers);

        // 创建线程
        int ret = pthread_create(&worker->threadid, NULL, nThreadPoolCallback, worker);
        if (ret) {
            perror("pthread_create");
            /*
            传入一级指针即可, pool 是指针; PriorityThreadPool 中的 works 也是指针
            传入 PriorityThreadPool 类型的指针, 就是拿到了结构体中 works 的二级指针
            */
            LIST_REMOVE(worker, pool->workers);  

            free(worker);
            return -3;
        }
    }  

    printf("[✓] 创建优先级线程池成功，工作线程数: %d\n", numWorkers);
    return 0;
}

// 推送任务
int nPriorityThreadPoolPushTask(PriorityThreadPool *pool, struct nTask *task) {
    if (pool == NULL || task == NULL) return -1;

    // 检查优先级范围
    if (task->priority < 1 || task->priority > NUM_PRIORITY_LEVELS) {
        printf("[✗] 错误：优先级 %d 超出范围 [1, %d]\n", task->priority, NUM_PRIORITY_LEVELS);
        return -2;
    }

    pthread_mutex_lock(&pool->mutex);

    // 根据优先级选择对应的队列（优先级 1→索引 0，优先级 10→索引 9）
    int queue_index = task->priority - 1;
    LIST_INSERT(task, pool->tasks[queue_index]);

    // 唤醒一个等待的线程
    pthread_cond_signal(&pool->cond);

    pthread_mutex_unlock(&pool->mutex);

    return 0;
}

// 销毁线程池
int nPriorityThreadPoolDestroy(PriorityThreadPool *pool) {
    if (pool == NULL) return -1;

    struct nWorker *worker = NULL;

    // 标记所有工作线程下班
    pthread_mutex_lock(&pool->mutex);
    for (worker = pool->workers; worker != NULL; worker = worker->next) {
        worker->terminate = 1;
    }
    pthread_mutex_unlock(&pool->mutex);

    // 唤醒所有等待的线程
    pthread_cond_broadcast(&pool->cond);

    // 等待所有线程结束
    for (worker = pool->workers; worker != NULL; worker = worker->next) {
        pthread_join(worker->threadid, NULL);
    }

    // 清理工作线程链表
    struct nWorker *curr = pool->workers;
    while (curr != NULL) {
        struct nWorker *next = curr->next;
        free(curr);
        curr = next;
    }

    // 清理所有任务队列中的任务
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        struct nTask *task = pool->tasks[i];
        while (task != NULL) {
            struct nTask *next = task->next;
            free(task);
            task = next;
        }
    }

    // 销毁锁和条件变量
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);

    printf("[✓] 销毁线程池成功\n");
    return 0;
}
