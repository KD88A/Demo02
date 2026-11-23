# Demo02 STM32F407 PWM 与 SG90 学习手册

从 **PA6 输出固定 PWM** 开始，依次完成 LED 三档调光、SG90 三位置定位和往返扫动。`Demo02.ioc` 已经配置好第一关的参数，`Core/Src/main.c` 也已生成 `MX_TIM3_Init()`。**现在直接做任务 1 的 PWM 启动步骤。**

## 先认识当前工程

| 文件 | 这次要看什么 |
| --- | --- |
| [Demo02.ioc](Demo02.ioc) | `TIM3` 使用内部时钟，`PWM Generation CH1` 映射到 `PA6`。工程目标工具链是 CMake。 |
| [Core/Src/main.c](Core/Src/main.c) | `main()` 已调用 `MX_TIM3_Init()`。在 `USER CODE 2` 启动 PWM，在 `StartDefaultTask()` 的 `USER CODE 5` 写变化逻辑。 |
| [Core/Src/stm32f4xx_hal_msp.c](Core/Src/stm32f4xx_hal_msp.c) | `HAL_TIM_MspPostInit()` 将 `PA6` 配成 `GPIO_AF2_TIM3`。探头、外接 LED 和舵机信号线都接这个引脚。 |
| [Core/Inc/FreeRTOSConfig.h](Core/Inc/FreeRTOSConfig.h) | FreeRTOS 为 1000 tick/s，`osDelay(1000U)` 约 1 秒。`TIM7` 提供 HAL 时基，PWM 由 `TIM3` 负责。 |
| [CMakePresets.json](CMakePresets.json) | `Debug` 预设使用 Ninja，构建目录为 `build/Debug`。 |

## 看懂 CubeMX 的 TIM3 参数

新工程的 `SYSCLK` 和 `HCLK` 都是 **84 MHz**。APB1 分频为 2，`PCLK1` 为 **42 MHz**，CubeMX 中的 **APB1 Timer clocks** 为 **84 MHz**。用最后这个值算 TIM3 的计数速度。 [STM32F407 参考手册 RM0090](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

| CubeMX 字段 | 任务 1–2（当前配置） | 任务 3–4 | 含义 |
| --- | --- | --- | --- |
| Prescaler（PSC） | 83 | 83 | 输入时钟除以 `83 + 1`，每格 1 µs |
| Counter Mode | Up | Up | 从 0 向上计数 |
| Counter Period（ARR） | 999 | 19999 | 一轮分别有 1000 格和 20000 格 |
| CH1 Mode | PWM mode 1 | PWM mode 1 | `CNT < CCR1` 时通道有效 |
| CH1 Pulse（CCR1 初值） | 500 | 1500 | 启动时有效脉冲的计数格数 |
| CH Polarity | High | High | 有效时 PA6 输出高电平。选 `Low` 会对调高低电平 |
| Fast Mode | Disable | Disable | 用于缩短触发到输出的响应时间。本实验连续输出 PWM，保持 `Disable` |

`Output compare preload` 保持启用。HAL 配置 PWM 通道时会启用 CH1 的比较值预装载，运行时写入的 `CCR1` 在下一次更新事件生效。`Auto-reload preload` 是 ARR 的独立选项，本实验运行时只改 `CCR1`，可保留当前的 `Disable`。主从模式和 TRGO 也保留生成值。 [STM32F407 参考手册 RM0090，TIM2–TIM5 的 CCMR1 和 CCER](https://www.st.com/resource/zh/reference_manual/DM00031020.pdf) · [工程中的 HAL 定时器驱动](Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c)

```text
计数频率 = 84 MHz / (83 + 1) = 1 MHz
PWM 周期 = (ARR + 1) × 1 µs
高电平时间 = CCR1 × 1 µs
占空比 = CCR1 / (ARR + 1)
```

按当前参数，`CNT = 0…499` 时 PA6 为高，`CNT = 500…999` 时为低。若把 `Pulse` 设为 0，通道启动后 PA6 会一直为低。

## 任务 1 让 PA6 输出 1 kHz PWM

1. 打开 `Core/Src/main.c`。在 `main()` 中找到 `MX_TIM3_Init();`，把下面的代码放进紧随其后的 `USER CODE BEGIN 2` 和 `USER CODE END 2` 之间。保留这段启动代码供后面三关使用。

   ```c
   if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
   {
       Error_Handler();
   }
   ```

2. 在 VS Code 打开 `Demo02` 文件夹。项目的 [VS Code 设置](.vscode/settings.json) 已配置使用 `cube-cmake`，可以用 CMake Tools 选择 `Debug` 预设并执行配置、构建。若终端已配置 CMake、Ninja 和 ARM GCC，也可以在工程根目录运行以下两条命令。STM32 扩展终端若提供的是 `cube-cmake`，把下面的 `cmake` 换成 `cube-cmake`。

   ```powershell
   cmake --preset Debug
   cmake --build --preset Debug
   ```

3. 构建成功后，通过 ST-LINK 用 STM32CubeProgrammer 下载 `build/Debug/Demo02.elf` 并运行。先把舵机留在断开状态。示波器或逻辑分析仪的探头接 **PA6**，地线接开发板 **GND**。

**验收**　PA6 周期约 **1 ms**，高电平约 **500 µs**，即 **1 kHz、50% 占空比**。没有仪器时，可给 PA6 接上下一关的 LED，通过三档亮度确认输出会变化。

**自测**　若 `ARR` 改为 `1999`、`CCR1` 仍为 `500`，周期变为 `2 ms`，高电平仍为 `500 µs`，占空比变为 `25%`。

## 任务 2 用 CCR1 实现 LED 三档调光

1. 断电后接线。`PA6 → 330 Ω 电阻 → LED 正极`，`LED 负极 → 开发板 GND`。
2. 保持第一关的 TIM3 参数，把 `StartDefaultTask()` 中 `USER CODE BEGIN 5` 和 `USER CODE END 5` 之间的空闲循环换成下面的代码，然后构建并烧录。

```c
const uint32_t compare_ticks[] = {100U, 500U, 900U};

for (;;)
{
    for (uint32_t i = 0; i < sizeof(compare_ticks) / sizeof(compare_ticks[0]); ++i)
    {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, compare_ticks[i]);
        osDelay(1000U);
    }
}
```

**验收**　LED 约每秒换一档，按暗、中、亮循环。三个档位的占空比约为 **10%、50%、90%**。用仪器测量时，高电平约为 **100、500、900 µs**，每档的 PWM 周期都是 **1 ms**。`osDelay()` 决定多久换档，TIM3 硬件持续重复输出当前档位。

## 任务 3 让 SG90 回中并试三个位置

### 先测固定中位脉冲

1. 打开 `Demo02.ioc`，把 TIM3 的 **Counter Period 改为 `19999`，CH1 Pulse 改为 `1500`**。保存并生成代码。检查 `MX_TIM3_Init()` 中 `htim3.Init.Period` 为 `19999`、`sConfigOC.Pulse` 为 `1500`，并确认 `USER CODE 2` 中的 PWM 启动调用仍在。
2. 把 `USER CODE 5` 恢复为空闲循环 `for (;;) { osDelay(1U); }`，移除调光逻辑，构建并烧录。接舵机前先核对 PA6 输出 **20 ms 周期（50 Hz）、1.5 ms 高电平**。有仪器时优先实测。
3. 断电拆下外接 LED，把舵机接好，再空载上电。给舵臂留出转动空间。

| 舵机端 | 接法 |
| --- | --- |
| 电源，常见红线 | 接独立稳压电源正端，电压按手上型号规格。TowerPro SG90 Analog 的标称工作电压为 4.8 V。 |
| 地，常见棕线或黑线 | 接外部电源负端，并与开发板 GND 共地。 |
| 信号，常见橙线或黄线 | 接 PA6。核对实物线序，并确认舵机能识别 3.3 V 控制信号。 |

舵机由独立电源供电。1.5 ms 是本实验用于寻找中位的起点，位置和方向以实物为准。持续嗡鸣或舵臂顶住时，先断电检查机械行程。 [TowerPro SG90 Analog 规格](https://towerpro.com.tw/product/sg90-analog/) · [SG90 脉宽时序](https://storage.googleapis.com/mauser-public-images/prod_description_document/2021/263/a08eb73a5be0b6a83b569bde59d55b27_sg90_datasheet.pdf)

### 再试中位两侧

中位稳定后，用下面的代码替换 `USER CODE 5` 的内容，再构建、烧录。此时每格 1 µs，所以比较值可以直接写成脉宽的微秒数。

```c
const uint32_t positions_us[] = {1500U, 1300U, 1500U, 1700U};

for (;;)
{
    for (uint32_t i = 0; i < sizeof(positions_us) / sizeof(positions_us[0]); ++i)
    {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, positions_us[i]);
        osDelay(1000U);
    }
}
```

**验收**　舵臂约每秒换一次位置，按「中位 → 一侧 → 中位 → 另一侧」循环。在纸上标记三个位置。`1500 / 20000 = 7.5%` 是中位脉冲在一帧中的占空比，实验中用 **1500 µs** 记录该位置。

## 任务 4 做往返扫动

保持任务 3 的 50 Hz 配置。确认 `1300 µs` 和 `1700 µs` 对实物都在可用行程内。若上一关收窄过范围，把下面的两个边界改为实测可用值。

1. 在 `USER CODE BEGIN 4` 和 `USER CODE END 4` 之间添加这些定义。

   ```c
   enum { SERVO_MIN_US = 1300, SERVO_CENTER_US = 1500, SERVO_MAX_US = 1700 };

   static void Servo_SetPulseUs(int32_t pulse_us)
   {
       if (pulse_us < SERVO_MIN_US)
       {
           pulse_us = SERVO_MIN_US;
       }
       else if (pulse_us > SERVO_MAX_US)
       {
           pulse_us = SERVO_MAX_US;
       }

       __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (uint32_t)pulse_us);
   }
   ```

2. 把下面的代码放进 `USER CODE 5`，替换三位置实验的循环。构建并烧录。

   ```c
   int32_t pulse_us = SERVO_CENTER_US;
   int32_t step_us = 10;

   for (;;)
   {
       Servo_SetPulseUs(pulse_us);
       osDelay(40U);

       if (pulse_us >= SERVO_MAX_US || pulse_us <= SERVO_MIN_US)
       {
           step_us = -step_us;
       }
       pulse_us += step_us;
   }
   ```

**验收**　舵臂从中位出发，在两端之间往返。每约 40 ms 改 10 µs，按 1300 到 1700 µs 计算，端点到端点约 **1.6 秒**。把 `osDelay(40U)` 改为 `osDelay(80U)` 再试，单程约 **3.2 秒**，PA6 的 PWM 周期仍约 **20 ms**。

## 按现象排查

| 现象 | 检查顺序 |
| --- | --- |
| PA6 没有 PWM | 确认程序已烧录并在运行 → 量 PA6 并共地 → 检查 `MX_TIM3_Init()` 和 `HAL_TIM_PWM_Start()` 已执行 → 核对 `Pulse` 大于 0。 |
| 周期和预期不符 | 检查 APB1 **定时器**时钟是不是 84 MHz，再用 `PSC + 1` 和 `ARR + 1` 重算。 |
| LED 一直同一亮度 | 检查 `StartDefaultTask()` 是否执行到 `__HAL_TIM_SET_COMPARE()`，再确认已烧录最新的 ELF。 |
| 舵机没有三个位置 | 先测信号线是否能看到 20 ms 周期和变化的高电平脉宽，再查共地、3.3 V 信号兼容性和舵机供电。 |
| 舵机动作时板子复位 | 检查舵机是否使用独立稳压电源，以及动作时电压是否下跌。 |

## 实验记录

| 任务 | 理论值 | 实测或观察 |
| --- | --- | --- |
| 固定 PWM | 1 ms 周期，500 µs 高电平 |  |
| LED 三档 | 100 / 500 / 900 µs，约每秒换档 |  |
| SG90 三位置 | 20 ms 周期，1300 / 1500 / 1700 µs | 记录三档实测脉宽和对应舵臂角度 |
| SG90 往返 | 默认单程约 1.6 秒 | 记录单程时间和实际行程 |
