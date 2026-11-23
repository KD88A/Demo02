# CMSIS-RTOS2 上手指南

**从 Demo02 的默认任务开始。** 第一遍先让调试器里的数字每秒加一，再用 PE4 的按键让 PA6 上的 LED 换档。做到这里，你就实际用过了线程、延时、信号量三组最常见的接口。

这里的 CMSIS 指 Arm 的 **CMSIS-RTOS2** 接口。Demo02 使用 FreeRTOS 执行这些调用。下面按 STM32 应用代码里通常会反复用到的程度排列，相关函数放在一起。内核启动函数虽然最先执行，但 CubeMX 通常已经写好，放到最后查阅。

| 想做什么 | 读哪一组 | 可以观察什么 |
| --- | --- | --- |
| 每隔一段时间执行 | 1 延时与节拍 | 一个变量每秒增加 |
| 让两件事独立进行 | 2 创建和查看线程 | 默认任务和新增任务各自休眠 |
| 按一下键唤醒任务 | 3 信号量 | 按键改变 PA6 的 PWM 占空比 |
| 把数值交给另一个任务 | 4 消息队列 | 每条指令的两项数据一起到达 |
| 两个任务修改同一份数据 | 5 互斥锁 | 两边的更新都被保留 |
| 唤醒指定任务，或等待多个条件 | 6 线程标志与事件标志 | 认出哪些事件已发生 |
| 到时执行一个短操作 | 7 软件定时器 | 定时器到期唤醒任务 |
| 看懂上电启动顺序 | 8 内核管理 | 找到初始化、创建和开始调度的位置 |

## 开始前先读懂这些参数

- **线程（任务）** 是一段由调度器安排执行的函数。每个线程有自己的栈，用来保存局部变量、函数调用和暂停时的现场。线程等待时间或事件时会进入阻塞状态，CPU 可以执行其他就绪线程。
- **句柄** 是创建对象时返回的标识。比如 `osSemaphoreId_t` 表示信号量句柄。保存它，后续调用就知道要操作哪一个对象。创建失败通常返回空指针 `NULL`。对象删除后旧句柄失效。
- **结构体和地址**。`osThreadAttr_t` 是把线程名字、优先级、栈大小等配置装在一起的类型。`.name` 选中一个成员，`&attr` 取整个结构体的地址。`const` 表示不能通过这个名字修改内容。创建接口的属性指针通常可以传 `NULL`，让系统使用默认配置。
- **函数指针和通用指针**。传任务入口时写函数名，如 `Worker`，调度器以后才调用它。`void *argument` 可以接收任意数据对象的地址。任务要读数据时，把它转回原来类型的指针，再用 `*` 取值。传给另一个任务的对象要活得足够久，后面的例子用 `static` 保证这一点。
- **时间**。tick 是内核节拍。`uint32_t` 是 32 位无符号整数，`100U` 中的 U 表示无符号。Demo02 每秒 1000 tick，所以 100 tick 约 100 ms。对支持超时的获取函数，`0U` 表示立即尝试，正数表示最多等几个 tick，`osWaitForever` 表示一直等到满足条件。
- **返回值**。`osStatus_t` 是状态码类型，成功值 `osOK` 等于 0。用“返回值等于 osOK”判断成功。`osErrorResource` 常见于立即尝试时资源不可用，`osErrorTimeout` 表示等到超时，`osErrorParameter` 表示参数有问题，`osErrorISR` 表示该调用不能在中断里使用。实际可能值随函数而异。返回句柄、数量或标志位的函数各有自己的判断方式。
- **中断与线程**。ISR 是中断服务程序。GPIO 边沿到来后，CPU 会暂停正在执行的代码进入中断。中断里只能使用明确允许的接口，也不能阻塞等待。任务优先级越高越先运行，而 STM32 的 NVIC 中断优先级数值越小，紧急程度越高。

### 在 Demo02 里放在哪里

每个实验从原始 `Demo02` 独立开始。一个实验有多段代码时按标注一起放入 [Core/Src/main.c](Core/Src/main.c)，再编译烧录。切换实验时移除前一实验的片段。文件已有 RTOS 头文件。保留 CubeMX 的 BEGIN / END 注释，只替换两者之间的内容。

| 本书标注 | 在 main.c 中搜索 | 用途 |
| --- | --- | --- |
| 区块 0 | `USER CODE BEGIN 0` | 放在 main 前的类型、文件内变量、任务函数、回调 |
| 区块 5 | `USER CODE BEGIN 5` | 替换默认任务原有的整个循环，内核此时已开始调度 |
| 观察变量 | VS Code 调试器的 Watch / 监视 | 构建选择 Debug，运行数秒后暂停，查看例中指定的变量 |

例中的 `volatile` 让编译器保留对观察变量的访问，适合调试器读值。多个任务共同修改数据时仍要同步，第 5 组会演示。例中使用工程自带的 `Error_Handler()` 停在异常位置，便于下断点查看调用栈。 [工程配置](Core/Inc/FreeRTOSConfig.h) · [函数声明及类型定义](Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.h)

## 1 延时和固定周期

从能看到结果的最小实验开始。相对延时从调用的时刻起算，绝对延时按预定时间表唤醒。

| 函数与用途 | 参数（按传入顺序） | 返回值 |
| --- | --- | --- |
| `osDelay` 暂停当前线程 | `ticks`，休眠的节拍数，通常传正数 | `osStatus_t`，正常为 `osOK`，在 ISR 调用为 `osErrorISR` |
| `osKernelGetTickCount` 读取当前时间 | 无 | `uint32_t`，启动后的节拍计数，会循环回到 0 |
| `osKernelGetTickFreq` 查询时间单位 | 无 | `uint32_t`，每秒 tick 数，Demo02 返回 1000 |
| `osDelayUntil` 等到指定时间点 | `ticks`，未来的绝对 tick 值 | `osStatus_t`，成功为 `osOK`，本工程目标已到或过期返回 `osErrorParameter` |

**区块 0**

```c
static volatile uint32_t seconds_seen;  // 调试器观察这个计数，静态变量初值为 0。
```

**区块 5**

```c
uint32_t next = osKernelGetTickCount();  // 读取起点，后面的唤醒时刻都从这里推算。
const uint32_t period = osKernelGetTickFreq();  // 一秒包含这么多个 tick。
for (;;) {  // 默认任务持续运行，每轮处理一次。
    next += period;  // 把目标推进一秒，即使本轮处理耗时，下一轮仍对齐原来的节奏。
    if (osDelayUntil(next) == osOK) {  // 等到这个绝对时刻，确认等待成功。
        ++seconds_seen;  // 在 Watch 中应看到计数大约每秒加一。
    } else {  // 单步调试或工作太久可能已经错过目标。
        next = osKernelGetTickCount();  // 从当前时间重新安排下一周期，避免连续追赶。
    }  // 完成本轮的成功或超时处理。
}  // 回到循环顶部。
```

若只要求做完一次后休息 100 tick，可直接调用 `osDelay(100U)`。一轮间隔会包含工作耗时。任务解除阻塞后，还要等调度器选中才会真正运行。当前适配层对 `osDelay(0U)` 直接返回，主动让同优先级线程运行可用 `osThreadYield()`。 [Arm 等待函数](https://arm-software.github.io/CMSIS_6/latest/RTOS2/group__CMSIS__RTOS__Wait.html)

## 2 线程的创建、入口参数和运行状态

一个线程负责采样，另一个线程可以同时等按键。调度器会在它们之间切换。先写入口函数，再把入口、数据地址和配置交给创建函数。

| 函数与用途 | 参数（按传入顺序） | 返回值 |
| --- | --- | --- |
| `osThreadNew` 创建线程 | `func` 入口函数，`argument` 交给入口的数据地址，`attr` 属性结构体地址或 `NULL` | `osThreadId_t` 线程句柄，失败为 `NULL` |
| `osThreadGetId` 获取自己 | 无 | 当前线程句柄，当前适配层在 ISR 返回 `NULL` |
| `osThreadGetState` 查看状态 | `thread_id`，要查看的线程句柄 | `osThreadState_t` 枚举，如就绪 `osThreadReady`、运行 `osThreadRunning`、阻塞 `osThreadBlocked`，出错为 `osThreadError` |
| `osThreadGetStackSpace` 检查栈余量 | `thread_id`，目标线程 | `uint32_t` 字节数，本工程按历史最小剩余栈计算，0 也可能表示参数无效 |
| `osThreadYield` 让出当前执行机会 | 无 | `osStatus_t`，成功为 `osOK`，适用于有同优先级就绪任务时 |
| `osThreadExit` 结束当前线程 | 无 | 无返回值，成功后不再继续执行该线程 |

**区块 0**

```c
static uint32_t sample_ticks = 100U;  // 为新任务长期保存采样间隔，单位是 tick。
static volatile uint32_t samples;  // 用调试器观察新增线程处理了几轮。
static const osThreadAttr_t worker_attr = {  // 定义只读属性，按成员名字赋值，未列出的成员置零。
    .name = "sample",  // 给调试器一个能辨认的线程名。
    .stack_size = 512U,  // 申请 512 字节线程栈，用于本例的简单局部变量和调用。
    .priority = osPriorityNormal  // 使用普通优先级，与 Demo02 默认任务相同。
};  // 属性结构体初始化完毕。
static void Worker(void *argument) {  // 声明符合 RTOS 要求的任务入口，参数由创建者传入。
    const uint32_t interval = *(const uint32_t *)argument;  // 把通用地址还原成整数地址，再读出周期值。
    for (;;) {  // 持续采样，直到系统停止。
        ++samples;  // 代替实际采样操作，留下可观察结果。
        osDelay(interval);  // 阻塞当前采样线程，让其他就绪线程运行。
    }  // 休眠结束后继续下一轮。
}  // 结束任务函数定义。
```

**区块 5**

```c
osThreadId_t worker = osThreadNew(Worker, &sample_ticks, &worker_attr);  // 传函数名和两个地址，保存新线程句柄。
if (worker == NULL) {  // 创建失败时不能继续使用这个句柄。
    Error_Handler();  // 在此下断点检查内存等创建条件。
}  // 创建检查结束。
for (;;) {  // 默认线程也要保留自己的工作循环。
    osDelay(1000U);  // 每秒醒来一次，新增线程在此期间仍能采样。
}  // 回到默认线程的循环起点。
```

**读懂 `osThreadAttr_t`。** `stack_mem` 是自备栈的首地址，`cb_mem` 和 `cb_size` 是内核控制块的地址及字节数。控制块存放调度状态等内部信息。这里由内核分配内存。`attr_bits` 放属性选项，`tz_module` 是 TrustZone 模块编号，`reserved` 为保留字段，本例用零。Demo02 配置的 RTOS 堆为 15360 字节，会由多个对象共享。`512` 只是这个简单示例的取值，增加大型局部数组或格式化输出后，应重新测量栈余量。观察 `samples`，约一秒增加 10 次。 [Arm 线程接口](https://arm-software.github.io/CMSIS_6/latest/RTOS2/group__CMSIS__RTOS__ThreadMgmt.html)

## 3 信号量让按键唤醒任务

把一次信号理解成一张可领取的票。二值信号量最多存一张，计数信号量可以存多张。按键中断发票，任务领到票后才切换亮度。

| 函数与用途 | 参数（按传入顺序） | 返回值 |
| --- | --- | --- |
| `osSemaphoreNew` 创建 | `max_count` 最大票数，`initial_count` 初始票数（0 到最大值），`attr` 属性地址或 `NULL` | `osSemaphoreId_t`，失败 `NULL` |
| `osSemaphoreAcquire` 领一张 | `semaphore_id` 句柄，`timeout` 最多等几个 tick | `osStatus_t`，领取成功 `osOK`，空且立即尝试为 `osErrorResource`，等到超时为 `osErrorTimeout` |
| `osSemaphoreRelease` 发一张 | `semaphore_id` 句柄 | `osStatus_t`，成功 `osOK`，票数已满 `osErrorResource` |
| `osSemaphoreGetCount` 查看现有票数 | `semaphore_id` 句柄 | `uint32_t` 当前票数，仅为读取时的快照 |
| `osSemaphoreDelete` 销毁 | `semaphore_id` 句柄，先让所有使用者和中断停止访问 | `osStatus_t`，成功 `osOK`，示例的常驻对象无需删除 |

**上板前的配置。** 这个实验需要在 CubeMX 的 NVIC 设置里把 **EXTI line4 的抢占优先级从 0 调为 5**，保存并重新生成代码，再加入下方两段代码。Demo02 当前 `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` 为 5，允许调用 RTOS 的 IRQ 抢占优先级数值为 5～15。保持 TIM3 的 ARR 为 999。如已在 `USER CODE 2` 启动 PWM，保留一次启动调用即可。

**区块 0**

```c
static volatile osSemaphoreId_t button_sem;  // 全局保存任务与中断共享的句柄，创建前为 NULL。
void HAL_GPIO_EXTI_Callback(uint16_t pin) {  // HAL 在处理外部中断时调用这个函数，参数是引脚掩码。
    if (pin == GPIO_PIN_4 && button_sem != NULL) {  // 只处理第 4 号引脚，且只在任务创建好信号量后通知。
        (void)osSemaphoreRelease(button_sem);  // 发一张票，此示例允许已有待处理票时合并通知，所以忽略满额结果。
    }  // 其他引脚或过早到达的中断直接跳过。
}  // 返回 HAL 的中断处理流程。
```

**区块 5**

```c
button_sem = osSemaphoreNew(1U, 0U, NULL);  // 最多一张，启动时零张，使用默认属性。
if (button_sem == NULL) {  // 判断信号量是否分配成功。
    Error_Handler();  // 创建失败时停下检查。
}  // 完成信号量检查。
if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK) {  // 启动已配置到 PA6 的 TIM3 通道 1，只启动一次。
    Error_Handler();  // PWM 启动失败则停止实验。
}  // 完成定时器检查。
uint32_t pulse = 100U;  // 记录当前高电平格数，100 格对应 10% 占空比。
__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);  // 把初始亮度写进 CCR1。
for (;;) {  // 等待每个待处理按键事件。
    if (osSemaphoreAcquire(button_sem, osWaitForever) == osOK) {  // 没有票就阻塞，被唤醒并领票后才执行。
        pulse = 1000U - pulse;  // 在 100 和 900 格之间切换。
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);  // 更新 PA6 的 PWM 占空比。
    }  // 一次事件处理完毕。
}  // 回去等待下一次通知。
```

让 PE4 收到上升沿，用示波器测 PA6 或接限流电阻和 LED 观察。机械按键抖动可能产生多个边沿，二值信号量只合并尚未消费的票，不能保证一次物理按压恰好切换一次。 [Arm 信号量接口](https://arm-software.github.io/CMSIS_6/latest/RTOS2/group__CMSIS__RTOS__SemaphoreMgmt.html)

## 4 用消息队列传递一整条指令

信号量能告诉接收方“有事发生”，消息队列还能保存“发生了什么”。这里让发送任务把序号和 PWM 比较值作为一条消息交给默认任务。

| 函数与用途 | 参数（按传入顺序） | 返回值 |
| --- | --- | --- |
| `osMessageQueueNew` 创建队列 | `msg_count` 最多几条，`msg_size` 每条多少字节，`attr` 属性地址或 `NULL` | `osMessageQueueId_t`，失败 `NULL` |
| `osMessageQueuePut` 复制一条消息入队 | `mq_id`、源数据地址 `msg_ptr`、优先级 `msg_prio`、等待空位的 `timeout` | `osStatus_t`，成功 `osOK`，满且立即尝试为 `osErrorResource`，超时为 `osErrorTimeout` |
| `osMessageQueueGet` 复制并移除队首消息 | `mq_id`、目标缓冲区地址、接收优先级的指针或 `NULL`、等待数据的 `timeout` | `osStatus_t`，成功 `osOK`，空且立即尝试为 `osErrorResource`，超时为 `osErrorTimeout` |
| `osMessageQueueGetCount` / `osMessageQueueGetSpace` 查看用量 | 各传一个 `mq_id` | `uint32_t`，分别返回已排队条数和剩余空位 |
| `osMessageQueueDelete` 销毁队列 | `mq_id`，先停止生产者和消费者 | `osStatus_t`，成功 `osOK` |

**区块 0**

```c
typedef struct {  // 定义一条调光指令包含哪些字段。
    uint32_t sequence;  // 第几条指令。
    uint32_t pulse;  // 想写入 CCR1 的比较值。
} LightCommand;  // 为这个结构体类型取名 LightCommand。
static osMessageQueueId_t commands;  // 两个线程共享同一个队列句柄。
static volatile LightCommand last_command;  // 保存最终收到的完整消息，供 Watch 查看。
static void SendCommands(void *argument) {  // 定义发送线程的入口。
    (void)argument;  // 显式说明本例没有使用启动参数。
    LightCommand cmd = {0U, 100U};  // 按字段顺序初始化序号和亮度。
    for (;;) {  // 不断生成下一条指令。
        ++cmd.sequence;  // 给这条消息设置新序号。
        if (osMessageQueuePut(commands, &cmd, 0U, osWaitForever) != osOK) {  // 将整个结构体复制入队，满时等待。
            Error_Handler();  // 记录这次发送失败的位置。
        }  // 结束发送检查。
        cmd.pulse = 1000U - cmd.pulse;  // 改写本地下一条指令，队列中已有副本不受影响。
        osDelay(500U);  // 每隔约半秒发一条。
    }  // 进入下一次发送。
}  // 结束发送线程定义。
```

**区块 5**

```c
commands = osMessageQueueNew(4U, sizeof(LightCommand), NULL);  // 申请四个槽，每槽正好容纳一个结构体。
if (commands == NULL) {  // 在启动使用者之前确认队列存在。
    Error_Handler();  // 内存不足等错误会停在这里。
}  // 完成队列检查。
if (osThreadNew(SendCommands, NULL, NULL) == NULL) {  // 创建发送线程，采用默认属性，不传启动参数。
    Error_Handler();  // 创建失败时停止。
}  // 成功后两个任务可以独立运行。
LightCommand received;  // 在本线程栈上预留完整结构体的空间。
for (;;) {  // 默认任务负责接收。
    if (osMessageQueueGet(commands, &received, NULL, osWaitForever) == osOK) {  // 等到消息，复制到 received，忽略优先级。
        last_command = received;  // 一次保存两个字段，可在调试器展开查看。
    }  // 只有成功取出时才读取 received。
}  // 回去接收下一条。
```

验收时展开 `last_command`，序号递增，`pulse` 在 100 和 900 之间切换。这个实验用调试器接收指令，不要求接 LED。Demo02 的 FreeRTOS 适配层忽略消息优先级，按入队顺序取出。若消息中保存的是指针，队列只复制地址，指向的数据还得由发送者和接收者约定生命周期。 [Arm 消息队列接口](https://arm-software.github.io/CMSIS_6/latest/RTOS2/group__CMSIS__RTOS__Message.html)

## 5 用互斥锁保护共享数据

两个任务都执行加一时，CPU 可能在读取旧值和写入新值之间切换任务，导致少加一次。让所有修改者先取得同一把锁，再完成整次更新。

| 函数与用途 | 参数（按传入顺序） | 返回值 |
| --- | --- | --- |
| `osMutexNew` 创建锁 | `attr` 属性结构体地址或 `NULL` | `osMutexId_t`，失败 `NULL` |
| `osMutexAcquire` 上锁 | `mutex_id`、`timeout` 最长等待 tick | `osStatus_t`，成功 `osOK`，立即尝试失败 `osErrorResource`，超时 `osErrorTimeout` |
| `osMutexRelease` 解锁 | `mutex_id`，由持锁线程调用 | `osStatus_t`，成功 `osOK`，未持有该锁可返回 `osErrorResource` |
| `osMutexGetOwner` 查看持有者 | `mutex_id` | `osThreadId_t`，无持有者或出错为 `NULL` |
| `osMutexDelete` 销毁 | `mutex_id`，确认已解锁且没有使用者 | `osStatus_t`，成功 `osOK` |

**区块 0**

```c
static osMutexId_t counter_lock;  // 保存两个任务共同使用的锁。
static volatile uint32_t total;  // 两个任务都要更新的总计数，初值为零。
static void CountWork(void *argument) {  // 两个线程可以使用同一个入口，各自有独立的栈。
    (void)argument;  // 这次不用启动参数。
    for (uint32_t i = 0U; i < 100U; ++i) {  // 每个线程完成一百次更新，i 是该线程私有的局部变量。
        if (osMutexAcquire(counter_lock, osWaitForever) != osOK) {  // 取得独占访问权，有人持有时就在此等待。
            Error_Handler();  // 没拿到锁，不能继续修改共享数据。
        }  // 完成加锁检查。
        ++total;  // 在持锁范围内读、加一、写回。
        if (osMutexRelease(counter_lock) != osOK) {  // 更新完成，由当前持有者解锁。
            Error_Handler();  // 停下检查解锁失败原因。
        }  // 完成解锁检查。
        osDelay(1U);  // 在锁外休息，等待期间其他任务可以更新计数。
    }  // 一百次更新已经完成。
    osThreadExit();  // 结束本线程，避免任务入口直接返回。
}  // 结束公共入口定义。
```

**区块 5**

```c
counter_lock = osMutexNew(NULL);  // 创建一把具有默认属性的普通锁。
if (counter_lock == NULL) {  // 确认创建成功。
    Error_Handler();  // 创建失败时停止。
}  // 完成锁检查。
if (osThreadNew(CountWork, NULL, NULL) == NULL) {  // 启动一个工作线程执行一百次更新。
    Error_Handler();  // 没有成功创建工作线程就停止。
}  // 工作线程已就绪。
CountWork(NULL);  // 默认线程也执行一百次更新，最后由 CountWork 内部结束默认线程。
```

验收时观察 `total` 最终为 200。普通锁由获得它的线程释放，ISR 不使用 mutex。若同一线程确实需要嵌套获取同一把锁，可在 `osMutexAttr_t.attr_bits` 指定 `osMutexRecursive`，获取几次就释放几次。信号量可以由另一个线程或合规中断释放，更适合第 3 组的通知。 [Arm 互斥锁接口](https://arm-software.github.io/CMSIS_6/latest/RTOS2/group__CMSIS__RTOS__MutexMgmt.html)

## 6 用标志位等待一个或多个条件

一个 bit 可以代表一个条件。掩码 1 的二进制是 01，掩码 2 是 10，按位或得到 11，即同时选择两个条件。线程标志随线程存在，适合通知指定接收者。事件标志是有独立句柄的对象，可供多个任务使用。

| 函数与用途 | 参数（按传入顺序） | 返回值 |
| --- | --- | --- |
| `osThreadFlagsSet` 通知指定线程 | `thread_id` 接收线程、`flags` 要置位的掩码 | `uint32_t` 标志结果或错误码 |
| `osThreadFlagsWait` 等待自己的标志 | `flags` 掩码、`options` 选项、`timeout` 等待时间 | 成功返回清除前的标志，失败返回带错误位的值 |
| `osThreadFlagsClear` 清除自己的位 | `flags` 掩码 | 清除前的标志或带错误位的值 |
| `osEventFlagsNew` 创建事件组 | `attr` 属性地址或 `NULL` | `osEventFlagsId_t`，失败 `NULL` |
| `osEventFlagsSet` / `osEventFlagsClear` 设置或清除 | 各传 `ef_id` 句柄、`flags` 掩码 | 任务中 Set 返回设置后的标志，Clear 返回清除前的标志，失败带错误位 |
| `osEventFlagsWait` 等待事件组合 | `ef_id`、`flags`、`options`、`timeout` | 满足条件时返回清除前的标志，失败带错误位 |
| `osEventFlagsDelete` 销毁事件组 | `ef_id`，所有使用者已停止访问 | `osStatus_t`，成功 `osOK` |

**区块 0**

```c
static osEventFlagsId_t ready;  // 保存“设备就绪”事件组。
static volatile uint32_t observed;  // 用于观察等齐时的标志。
static void Prepare(void *argument) {  // 用一个工作线程模拟两项初始化先后完成。
    (void)argument;  // 不使用启动参数。
    if (osEventFlagsSet(ready, 1U) & osFlagsError) {  // 标记第一项已完成，并检查返回值的最高错误位。
        Error_Handler();  // 设置失败时停止。
    }  // 第一项标记已发布。
    osDelay(500U);  // 模拟第二项还需要约半秒。
    if (osEventFlagsSet(ready, 2U) & osFlagsError) {  // 标记第二项完成，并检查错误位。
        Error_Handler();  // 设置失败时停止。
    }  // 两项完成通知均已发出。
    osThreadExit();  // 初始化线程完成职责后退出。
}  // 结束模拟初始化函数。
```

**区块 5**

```c
ready = osEventFlagsNew(NULL);  // 用默认属性创建，初始标志均为零。
if (ready == NULL) {  // 检查对象是否存在。
    Error_Handler();  // 创建失败则停止。
}  // 完成创建检查。
if (osThreadNew(Prepare, NULL, NULL) == NULL) {  // 启动会陆续设置两个条件的任务。
    Error_Handler();  // 生产事件的线程必须成功创建。
}  // 初始化任务已启动。
uint32_t bits = osEventFlagsWait(ready, 1U | 2U, osFlagsWaitAll, osWaitForever);  // 等到两位都为 1，成功后默认清除所等位。
if (bits & osFlagsError) {  // 标志函数用最高位表示失败，不能和 osOK 比来判断成功。
    Error_Handler();  // 只在等待出错时停下。
}  // 完成标志检查。
observed = bits;  // 在调试器中看到低两位都置位，本例值为 3。
osThreadExit();  // 演示完成，结束默认线程。
```

`osFlagsWaitAny` 表示任一位满足即可，`osFlagsWaitAll` 要求选中的位全部满足。用按位或附加 `osFlagsNoClear` 可以保留标志，要留意下次等待可能马上返回。重复设置同一位会合并。Demo02 的线程标志可用位 0～30，事件组可用位 0～23。下一组用线程标志传递一次定时通知。 [Arm 线程标志](https://arm-software.github.io/CMSIS_6/latest/RTOS2/group__CMSIS__RTOS__ThreadFlagsMgmt.html) · [Arm 事件标志](https://arm-software.github.io/CMSIS_6/latest/RTOS2/group__CMSIS__RTOS__EventFlags.html)

## 7 用软件定时器定期通知任务

软件定时器到期时由 FreeRTOS 的定时器服务线程执行回调。回调适合短小的通知，耗时处理交给被唤醒的任务。输出微秒级 PWM 继续使用 TIM3 硬件。

| 函数与用途 | 参数（按传入顺序） | 返回值 |
| --- | --- | --- |
| `osTimerNew` 创建 | `func` 回调函数名，`type` 为单次 `osTimerOnce` 或周期 `osTimerPeriodic`，`argument` 回调参数，`attr` 属性地址或 `NULL` | `osTimerId_t`，失败 `NULL`，创建后还需启动 |
| `osTimerStart` 启动或重设周期 | `timer_id`、正数 `ticks`，到期前的节拍数 | `osStatus_t`，`osOK` 表示启动命令被接受 |
| `osTimerStop` 停止 | `timer_id` | `osStatus_t`，成功 `osOK`，未运行可返回 `osErrorResource` |
| `osTimerIsRunning` 查询状态 | `timer_id` | `uint32_t`，运行中为 1，否则为 0 |
| `osTimerDelete` 删除 | `timer_id`，应确保没有其他逻辑继续使用 | `osStatus_t`，成功 `osOK` |

**区块 0**

```c
static volatile uint32_t wakeups;  // 记录默认任务实际处理的到期通知次数。
static void OnTimer(void *argument) {  // 定时器服务线程调用这个回调，并交回创建时保存的参数。
    osThreadId_t target = (osThreadId_t)argument;  // 恢复默认任务的句柄，用来指定接收者。
    if (osThreadFlagsSet(target, 1U) & osFlagsError) {  // 设置接收者的第 0 位并检查是否失败。
        Error_Handler();  // 本实验中遇到无效通知就停下调试。
    }  // 通知结束，正常路径立即返回。
}  // 结束软件定时器回调。
```

**区块 5**

```c
osTimerId_t timer = osTimerNew(OnTimer, osTimerPeriodic, osThreadGetId(), NULL);  // 每次到期都把当前任务句柄传给 OnTimer。
if (timer == NULL) {  // 确认已经获得定时器对象。
    Error_Handler();  // 创建失败则停止。
}  // 完成创建检查。
if (osTimerStart(timer, 250U) != osOK) {  // 申请每 250 tick 到期一次。
    Error_Handler();  // 启动命令未被接受时停止。
}  // 定时通知即将开始。
for (;;) {  // 默认任务持续处理通知。
    uint32_t flags = osThreadFlagsWait(1U, osFlagsWaitAny, osWaitForever);  // 等自己的第 0 位，等到后默认清除此位。
    if (flags & osFlagsError) {  // 检查返回值的最高错误位。
        Error_Handler();  // 等待出错时停止。
    }  // 获得有效通知。
    ++wakeups;  // 任务处理一次，在本例中每秒约增加四次。
}  // 重新阻塞等待，所以空闲时不消耗 CPU 轮询。
```

把 `osTimerPeriodic` 改成 `osTimerOnce` 再运行。`wakeups` 加到 1 后，任务会一直等下一个通知，直到再次启动定时器。若处理者太慢，多次到期设置同一位可能合并，需要逐次累计时可选计数信号量。 [Arm 定时器接口](https://arm-software.github.io/CMSIS_6/latest/RTOS2/group__CMSIS__RTOS__TimerMgmt.html)

## 8 内核初始化、启动和状态

打开 main.c，从外设初始化往下读。CubeMX 已经生成下面三个阶段。

| 函数与用途 | 参数（按传入顺序） | 返回值 |
| --- | --- | --- |
| `osKernelInitialize` 让内核进入准备状态 | 无，在启动流程中调用一次 | `osStatus_t`，成功 `osOK`，已初始化时可返回 `osError` |
| `osKernelStart` 启动调度 | 无，先创建好初始任务和所需对象 | 声明为 `osStatus_t`，正常开始调度后由就绪线程接管，通常不会返回到 main 的下一行 |
| `osKernelGetState` 查看内核状态 | 无 | `osKernelState_t` 枚举，例如 `osKernelInactive` 尚未初始化、`osKernelReady` 可以创建初始任务、`osKernelRunning` 正在调度 |

**对照工程阅读，无需重复添加。**

```c
osKernelInitialize();  // main 初始化外设后准备内核，后面的创建操作才有运行环境。
defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);  // 注册默认线程，保存句柄，实际入口会在开始调度后执行。
osKernelStart();  // 启动调度，让就绪线程按优先级运行。
```

在 `StartDefaultTask` 开头下断点。若能命中，说明已经过初始化、默认线程创建和调度启动三个阶段。上电前半段需要所有任务共享的队列或锁，可放入 main 对应的 `RTOS_QUEUES` 或 `RTOS_MUTEX` 用户区块。 [Arm 内核接口](https://arm-software.github.io/CMSIS_6/latest/RTOS2/group__CMSIS__RTOS__KernelCtrl.html)

## 常见属性结构体和中断速查

| 类型 | 什么时候要自己填写 |
| --- | --- |
| `osThreadAttr_t` | 想指定线程名称、栈大小或优先级时，见第 2 组完整初始化。 |
| `osSemaphoreAttr_t`、`osTimerAttr_t`、`osEventFlagsAttr_t` | 常规实验直接传 `NULL`。需要名字时设 `.name`，需要指定控制块的存储时再填写 `cb_mem` 与 `cb_size`。这些类型的 `attr_bits` 保留为 0。 |
| `osMutexAttr_t` | 和上面一样有名字及控制块字段，另可用 `attr_bits` 指定锁的行为，如递归。 |
| `osMessageQueueAttr_t` | 除控制块外，还可用 `mq_mem` 和 `mq_size` 指定存放消息副本的缓冲区及字节数。第 4 组传 `NULL` 由系统分配。 |

| 在 Demo02 的中断里想做什么 | 可以怎么写 |
| --- | --- |
| 通知任务有按键边沿 | 用 `osSemaphoreRelease` 或 `osThreadFlagsSet`。必须先有有效句柄，IRQ 优先级满足第 3 组的条件。 |
| 获取信号量或传一条队列消息 | `osSemaphoreAcquire`、`osMessageQueuePut`、`osMessageQueueGet` 的 timeout 必须为 0，检查返回值决定如何处理满、空等状态。 |
| 等一段时间、上锁、等待线程标志或启动软件定时器 | 在任务里做。中断只记录情况并唤醒任务。Demo02 的适配层也会拒绝 ISR 中的 `osEventFlagsWait`。 |

**接着用原生 FreeRTOS 时。** 同一个任务里可以调用 `osDelay` 和 `xSemaphoreTake`。对同一个对象，创建、使用和删除尽量用同一套 API。原生获取信号量成功时返回 `pdTRUE`，CMSIS 获取成功返回 `osOK`，两者判断方式不同。Demo02 的线程标志底层还使用 FreeRTOS 任务通知，给同一个线程安排这两种通知时要避免争用同一份通知状态。 [Demo02 的 CMSIS 适配层](Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c)

**用三个问题检验理解。** 如果每一条按键事件都必须计数，先考虑计数信号量及其容量。如果每个事件还带一个不同的测量值，考虑消息队列。如果两项工作必须等到电源和传感器都就绪才开始，考虑事件组的两个位与 `osFlagsWaitAll`。这三种需求分别保留次数、数据和状态。

