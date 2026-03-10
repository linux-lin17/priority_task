# 优先级线程池项目 (Priority ThreadPool)

## 📌 项目概述

这是一个**优先级线程池**的完整实现，演示如何在多线程系统中根据任务优先级来调度执行。

### 核心特性
- ✅ 按优先级等级（1-10）管理任务
- ✅ 高优先级任务优先执行，不被低优先级阻塞
- ✅ 同一优先级任务按 FIFO 顺序执行
- ✅ 线程安全的队列管理（互斥锁 + 条件变量）
- ✅ 支持动态任务推送和并发执行

## 📂 项目结构

```
priority-task/
├── include/
│   └── priority_threadpool.h      # 头文件：API 定义和数据结构
├── src/
│   └── priority_threadpool.c      # 核心实现：线程池逻辑
├── main.c                          # 演示程序：10 个任务的执行
├── Makefile                        # 编译脚本
├── ANALYSIS.md                     # 详细分析文档（三个问题的答案）
└── README.md                       # 本文件

已编译的可执行文件：
├── priority_threadpool_app         # 演示程序的可执行文件
└── build/                          # 编译中间文件
```

## 🚀 快速开始

### 编译项目
```bash
cd /home/yuyang/share/04_threadpool/priority-task
make clean && make
```

### 运行程序
```bash
./priority_threadpool_app
```

### 清理
```bash
make clean
```

## 📊 演示内容

程序推送 **10 个任务**，分布如下：
- **3 个优先级为 1 的任务** (Task1, Task3, Task10)
- **1 个优先级为 2 的任务** (Task4)
- **1 个优先级为 3 的任务** (Task6)
- **1 个优先级为 4 的任务** (Task8)
- **1 个优先级为 5 的任务** (Task7)
- **1 个优先级为 6 的任务** (Task9)
- **2 个优先级为 7 的任务** (Task2, Task5)

### 推送顺序（FIFO）
```
Task1(1) → Task2(7) → Task3(1) → Task4(2) → Task5(7) 
→ Task6(3) → Task7(5) → Task8(4) → Task9(6) → Task10(1)
```

### 实际执行顺序（按优先级）
```
优先级 7: Task2 ✓ Task5 ✓     (第一优先级，立即执行)
优先级 6: Task9 ✓
优先级 5: Task7 ✓
优先级 4: Task8 ✓
优先级 3: Task6 ✓
优先级 2: Task4 ✓
优先级 1: Task1 ✓ Task3 ✓ Task10 ✓  (最低优先级，最后执行)
```

**关键观察**：虽然 Task1 被最先推送，但它优先级最低，所以被延迟执行，而 Task2 虽然第二个推送，但因为优先级最高（7），所以立即执行！

## 🏗️ 架构设计

### 数据结构

```c
typedef struct PriorityThreadPool {
    struct nWorker *workers;                   // 工作线程链表
    struct nTask *tasks[NUM_PRIORITY_LEVELS];  // 按优先级分类的任务队列
    
    pthread_mutex_t mutex;                     // 互斥锁
    pthread_cond_t cond;                       // 条件变量
    
    int num_workers;
    int shutdown;
} PriorityThreadPool;
```

### 关键特点

1. **多队列方案**（类似哈希表的拉链法）
   - `tasks[0]` → 优先级 1 的任务队列
   - `tasks[1]` → 优先级 2 的任务队列
   - ...
   - `tasks[9]` → 优先级 10 的任务队列

2. **任务出队算法**
   ```
   从优先级 10 到 1 依次检查：
   for (i = 9; i >= 0; i--) {
       if (tasks[i] 有任务) {
           摘下并执行
           break
       }
   }
   ```

3. **并发控制**
   - `mutex` 保护队列的修改
   - `cond` 实现高效的等待-唤醒机制

## 📈 性能指标

| 操作 | 时间复杂度 | 说明 |
|------|----------|------|
| 任务入队 | O(1) | 直接在队列头插入 |
| 任务出队 | O(P) | P=优先级等级数(10) |
| 等待通知 | O(1) | 条件变量，无忙轮询 |

## 💻 使用 API

### 创建线程池
```c
PriorityThreadPool pool;
nPriorityThreadPoolCreate(&pool, 3);  // 创建 3 个工作线程
```

### 推送任务
```c
struct nTask *task = malloc(sizeof(struct nTask));
task->task_func = my_task_function;
task->user_data = (void *)my_data;
task->priority = 7;  // 优先级 1-10

nPriorityThreadPoolPushTask(&pool, task);
```

### 销毁线程池
```c
nPriorityThreadPoolDestroy(&pool);
```

## 📚 详细分析

详见 [ANALYSIS.md](ANALYSIS.md) 文件，包含：

1. **问题1**：10 个任务的执行顺序分析
   - 详细的时间轴演示
   - 各个线程的行为

2. **问题2**：为什么说这是"拉链法"？
   - 与哈希表的类比
   - 数据结构优势

3. **问题3**：架构思路与执行过程
   - 初始化、推送、执行、销毁各阶段
   - 关键算法和伪代码

## 🔄 对比：单队列 vs 多队列

### ❌ 单队列方案（传统线程池）
```
tasks → [Task1(pri=1)] → [Task2(pri=7)] → [Task3(pri=1)]
           ↑
         先执行！

问题: 高优先级 Task2 被低优先级 Task1 阻塞
```

### ✅ 多队列方案（优先级线程池）
```
tasks[0] (优先级1) → [Task1] → [Task3]
tasks[1] (优先级2) → NULL
...
tasks[6] (优先级7) → [Task2]
           ↑
         先执行！

优点: Task2 立即执行，不被阻塞
```

## 🧪 测试结果

实际运行 `priority_threadpool_app` 的输出：

```
========================================
  优先级线程池演示程序
========================================

[✓] 创建优先级线程池成功，工作线程数: 3

  推送 Task1 (优先级=1)
  推送 Task2 (优先级=7)
  推送 Task3 (优先级=1)
  ...

  [执行中] 任务 ID=2, 优先级=7  ✓ 最高优先级先执行
  [执行中] 任务 ID=5, 优先级=7
  [执行中] 任务 ID=9, 优先级=6
  ...

  [销毁线程池]
[✓] 销毁线程池成功

========================================
  演示程序结束
========================================
```

## 🎓 学习目标

通过这个项目，你可以学习到：
- ✅ 多线程编程的基础（mutex, cond_wait, thread 等）
- ✅ 队列相关的数据结构设计
- ✅ 优先级调度的实现方式
- ✅ 临界区保护和同步机制
- ✅ 实际生产级别的线程池设计

## 🔗 文件说明

| 文件 | 说明 |
|------|------|
| `priority_threadpool.h` | API 定义、数据结构 |
| `priority_threadpool.c` | 核心实现：创建、推送、销毁 |
| `main.c` | 10 个任务的演示程序 |
| `ANALYSIS.md` | 三个问题的详细答案 |
| `Makefile` | 编译脚本 |

## 💡 扩展思路

1. **优化出队算法**：使用堆或跳表提高查询效率
2. **动态优先级调整**：任务在执行中调整优先级
3. **优先级反转**：防止低优先级任务永远被推迟
4. **任务超时**：添加任务执行超时机制
5. **负载均衡**：根据线程负载动态分配任务

## 📞 联系与问题

如有问题，请按照 ANALYSIS.md 中的详细说明进行理解。

---

**创建时间**：2026 年 3 月 10 日  
**版本**：1.0
