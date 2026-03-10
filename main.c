#include "priority_threadpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// 任务执行函数
void task_func(struct nTask *task) {
    int task_id = *(int *)task->user_data;
    
    printf("  [执行中] 任务 ID=%d, 优先级=%d, 线程ID=%lu\n", 
           task_id, task->priority, pthread_self());
    
    // 模拟任务执行时间
    sleep(1);
    
    printf("  [完成] 任务 ID=%d, 优先级=%d ✓\n", task_id, task->priority);
    
    // 清理资源
    free(task->user_data);
    free(task);
}

int main() {
    printf("========================================\n");
    printf("  优先级线程池演示程序\n");
    printf("========================================\n\n");

    // 创建线程池（只有 3 个工作线程）
    PriorityThreadPool pool;
    int ret = nPriorityThreadPoolCreate(&pool, 3);
    if (ret) {
        printf("创建线程池失败\n");
        return -1;
    }

    printf("\n【任务信息】\n");
    printf("  总任务数: 10\n");
    printf("  工作线程数: 3\n");
    printf("  优先级范围: 1-10 (数字越大优先级越高)\n\n");

    printf("【推送任务顺序（FIFO）】\n");
    
    // 定义任务：
    // - 3 个优先级为 1 的任务
    // - 1 个优先级为 2 的任务
    // - 1 个优先级为 3 的任务
    // - 1 个优先级为 4 的任务
    // - 1 个优先级为 5 的任务
    // - 1 个优先级为 6 的任务
    // - 2 个优先级为 7 的任务
    
    struct config {
        int task_id;
        int priority;
    } configs[] = {
        {1, 1},
        {2, 7},      // 高优先级，第一个被执行
        {3, 1},
        {4, 2},
        {5, 7},      // 高优先级
        {6, 3},
        {7, 5},
        {8, 4},
        {9, 6},
        {10, 1},
    };
    
    int num_tasks = sizeof(configs) / sizeof(configs[0]);

    // 推送所有任务
    for (int i = 0; i < num_tasks; i++) {
        struct nTask *task = (struct nTask *)malloc(sizeof(struct nTask));
        if (task == NULL) {
            perror("malloc");
            return -1;
        }
        memset(task, 0, sizeof(struct nTask));

        task->task_func = task_func;
        task->user_data = (int *)malloc(sizeof(int));
        *(int *)task->user_data = configs[i].task_id;
        task->priority = configs[i].priority;

        printf("  推送 Task%d (优先级=%d)\n", configs[i].task_id, configs[i].priority);

        ret = nPriorityThreadPoolPushTask(&pool, task);
        if (ret) {
            printf("推送任务失败\n");
            return -1;
        }

        // 推送任务之间稍作延迟，便于观察
        usleep(100000);
    }

    printf("\n【任务执行顺序分析】\n");
    printf("  预期执行顺序（优先级从高到低）:\n");
    printf("    优先级 7: Task2, Task5\n");
    printf("    优先级 6: Task9\n");
    printf("    优先级 5: Task7\n");
    printf("    优先级 4: Task8\n");
    printf("    优先级 3: Task6\n");
    printf("    优先级 2: Task4\n");
    printf("    优先级 1: Task1, Task3, Task10\n\n");

    printf("【实际执行过程】\n");
    printf("  (同一优先级的任务按 FIFO 顺序执行)\n\n");

    // 等待所有任务执行完毕
    sleep(15);  // 10 个任务 × 1 秒 + 一些缓冲时间

    printf("\n【销毁线程池】\n");
    nPriorityThreadPoolDestroy(&pool);

    printf("\n========================================\n");
    printf("  演示程序结束\n");
    printf("========================================\n");

    return 0;
}
