# LCD 触摸屏控制实现说明

当前工程已经加入 XPT2046 电阻触摸屏控制。串口控制仍然保留，触摸输入只是新增的并行控制方式。

## 已实现内容

- 新增 `BSP/bsp_touch.c` 和 `BSP/bsp_touch.h`。
- 在 `main.c` 中初始化触摸屏，并在主循环中扫描触摸事件。
- 在 LCD 开始界面底部绘制 `START` 和 `MODE` 虚拟触摸按键。
- 游戏中改为全屏滑动控制，将滑动事件转换为现有游戏命令，复用 `snake_game_on_command()`。
- 已将 `bsp_touch.c` 加入 Keil 工程文件 `snake.uvprojx`。
- 支持游戏中全屏滑动控制，松手后根据滑动方向改变蛇的方向。

## 当前触摸逻辑

开始界面和游戏结束界面使用底部两个虚拟按键：

```text
上方：FULL SCREEN SWIPE 提示
下方：MODE | 空白 | START
```

按键区域按照 LCD 屏幕坐标判断：

- `MODE`：x=0..79，y=272..319
- 中间空白：x=80..159，y=272..319，不触发事件
- `START`：x=160..239，y=272..319

进入游戏后不再检测方向按钮，也不限制触摸区域，还不依赖换算后的 `xy` 坐标。触摸驱动只记录手指在屏幕任意位置按下时的 raw 坐标和松手前最后一次 raw 坐标，根据 raw 差值判断滑动方向：

- 向上滑：输出 `TOUCH_EVENT_UP`
- 向下滑：输出 `TOUCH_EVENT_DOWN`
- 向左滑：输出 `TOUCH_EVENT_LEFT`
- 向右滑：输出 `TOUCH_EVENT_RIGHT`

这些事件在 `main.c` 中转换为 `snake_game_on_command('U'/'D'/'L'/'R')`，因此和串口方向键是同一套绝对方向逻辑，不是左转/右转逻辑。

## 引脚分配

XPT2046 触摸屏使用以下引脚：

- PENIRQ：PE4
- CS：PD13
- CLK：PE0
- MOSI：PE2
- MISO：PE3

当前工程其他主要引脚：

- LCD FSMC 数据/控制线：PD0、PD1、PD4、PD5、PD7、PD8、PD9、PD10、PD11、PD14、PD15、PE7 到 PE15
- LCD 复位：PE1
- LCD 背光：PD12
- USART1：PA9、PA10
- LED/BEEP：PB5、PA8
- 板载按键：PA0、PC13

从引脚分配看，触摸屏与当前主要外设没有直接冲突。

## 坐标校准

当前代码中仍保留 `touch_raw_to_screen()`，但它只用于开始界面和游戏结束界面的 `START`、`MODE` 按钮点击。

如果 `START` 或 `MODE` 点击位置明显偏移，需要重新校准以下逻辑：

```text
BSP/bsp_touch.c
touch_raw_to_screen(...)
```

建议调试方法：

1. 点击屏幕底部左侧 `MODE`，确认串口出现 `[Touch] event=MODE`。
2. 点击屏幕底部右侧 `START`，确认串口出现 `[Touch] event=START`。
3. 如果按钮偏移，根据实际日志中的 `raw` 和 `xy` 修正 `touch_raw_to_screen()`。
4. 进入游戏后在屏幕任意位置分别测试上下左右滑动，确认串口出现 `[Touch] swipe ... event=UP/DOWN/LEFT/RIGHT`。
5. 如果滑动方向反了，优先调整 `touch_swipe_to_event()` 中 raw 到屏幕方向的映射。

## 注意事项

- 当前触摸驱动是寄存器级实现，不依赖 STM32 标准外设库。
- 触摸采样带有简单去抖。菜单按钮同一次按住只触发一次事件；游戏滑动在松手时触发一次方向事件。
- 如果触摸屏未连接或 PENIRQ 引脚一直为高电平，不会影响串口控制。
- 如果触摸屏硬件版本不同，最可能需要调整的是坐标校准参数，而不是游戏逻辑。

## 当前推荐演示方式

1. 上电进入开始界面。
2. 使用触摸 `MODE` 切换普通模式和障碍物模式。
3. 使用触摸 `START` 开始游戏。
4. 进入游戏后，在屏幕任意位置上下左右滑动控制蛇移动。
5. 同时打开上位机网页仪表盘观察分数、时长和历史记录。
