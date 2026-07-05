# STM32F103 贪吃蛇项目说明

## 编译方式

使用 Keil MDK 打开工程文件 `snake.uvprojx`，选择 `Target_1` 后编译。

当前工程面向野火指南者风格的 STM32F103VE 开发板。LCD 驱动使用 FSMC 8080 并口方式连接 ILI9341/ST7789V 屏幕。

内部 Flash 最后 2 KB 预留给最高分存储：

- 程序 IROM 范围：`0x08000000` 到 `0x0807F7FF`
- 最高分存储页：`0x0807F800`

## 串口方向控制

如果电脑端没有安装 `pyserial`，先执行：

```bash
pip install pyserial
```

进入 `Tools` 目录后运行：

```bash
python pc_arrow_control.py COM3
```

其中 `COM3` 需要替换为实际串口号。

控制方式：

- 方向键上：蛇向上移动
- 方向键下：蛇向下移动
- 方向键左：蛇向左移动
- 方向键右：蛇向右移动
- Enter：开始游戏或重新开始
- `B` 或 `M`：在开始界面/游戏结束界面切换普通模式和障碍物模式
- Esc：退出电脑端控制程序

电脑端控制程序会向 USART1 发送单字节命令：

- `U`：上
- `D`：下
- `L`：左
- `R`：右
- `S`：开始/重新开始
- `B`：切换游戏模式

## LCD 触摸屏控制

当前触摸屏控制基于 XPT2046 电阻触摸屏，串口控制仍然保留，两种输入可以并行使用。

开始界面和游戏结束界面只保留两个触摸按钮：

```text
上方：FULL SCREEN SWIPE 提示
下方：MODE | 空白 | START
```

按钮功能：

- `MODE`：在普通模式和障碍物模式之间切换。
- `START`：开始游戏或重新开始。

进入游戏后不再使用屏幕底部方向按钮，改为全屏滑动控制。手指可以从屏幕任意位置按下并滑动，松手时根据滑动方向输出绝对方向命令：

- 向上滑：等价于串口命令 `U`。
- 向下滑：等价于串口命令 `D`。
- 向左滑：等价于串口命令 `L`。
- 向右滑：等价于串口命令 `R`。

调试时可以在 USART1 看到触摸日志：

```text
[Touch] press raw=520,901 xy=185,296
[Touch] event=START
[Touch] swipe-start raw=3000,2200
[Touch] swipe raw-start=3000,2200 raw-end=1800,2200 event=DOWN
```

游戏中的滑动识别直接使用 raw 起点和终点，不依赖 LCD 坐标校准；`START` 和 `MODE` 点击仍需要 `touch_raw_to_screen()` 做位置换算。

## 上位机网页可视化

固件会通过 USART1 输出游戏数据事件，格式如下：

```text
SNAKE,START,mode=normal
SNAKE,FOOD,score=30,length=7,duration=12
SNAKE,END,score=80,high=120,duration=45,reason=wall
```

事件含义：

- `START`：一局游戏开始
- `FOOD`：蛇吃到食物，分数和长度发生变化
- `END`：游戏结束，输出最终得分、最高分、游戏时长和失败原因

运行网页仪表盘：

```bash
python snake_web_dashboard.py COM3
```

浏览器打开：

```text
http://127.0.0.1:8080
```

网页仪表盘显示内容：

- 当前分数
- 历史最高分
- 本局游戏时长
- 当前游戏模式和状态
- 历史得分排名
- 每局游戏时长统计
- 最近串口事件记录

网页仪表盘所在的控制台也支持方向键、Enter、`B`/`M` 和 Esc 控制，可替代 `pc_arrow_control.py` 用于演示。

## 游戏模式

当前支持两种模式：

- 普通模式：经典贪吃蛇玩法。
- 障碍物模式：棋盘上会出现固定障碍物，蛇撞到障碍物后游戏结束。

在开始界面或游戏结束界面按 `B` 或 `M` 可以切换模式。

## 触摸屏可行性

触摸屏控制已经接入工程。当前默认使用野火指南者 3.2 寸电阻触摸屏常见校准参数。

触摸屏接入方案、引脚分析和后续校准建议见：

```text
Docs/touchscreen_feasibility.md
```

## 硬件连接说明

- USART1：PA9 为 TX，PA10 为 RX，波特率 115200，8N1。
- LCD：ILI9341/ST7789V，FSMC 8080 并口连接，参考野火指南者 LCD 例程接线。
- LED/BEEP：红色 LED 使用 PB5，蜂鸣器使用 PA8。
- 板载按键：KEY1/PA0 可开始游戏或左转；KEY2/PC13 可开始游戏或右转。

如果 LCD 白屏或显示异常，优先确认：

- 开发板是否为带 FSMC LCD 的版本。
- Keil 工程目标芯片是否与实际 MCU 型号一致。
- LCD 排线和供电是否正常。
