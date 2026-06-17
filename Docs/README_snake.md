# STM32F103 Snake

## Build

Open `snake.uvprojx` with Keil MDK and build target `Target_1`.

The project is configured for the Wildfire Guide style STM32F103VE board because the LCD code uses FSMC pins for the ILI9341 screen. The last 2 KB of internal Flash is reserved for the high-score record:

- Application IROM: `0x08000000` to `0x0807F7FF`
- High score page: `0x0807F800`

## PC Arrow Controller

Install pyserial if needed:

```bash
pip install pyserial
```

Run:

```bash
python pc_arrow_control.py COM3
```

Use:

- Arrow Up: move up
- Arrow Down: move down
- Arrow Left: move left
- Arrow Right: move right
- Enter: start or restart
- Esc: quit the PC helper

The PC helper sends single-byte commands to USART1:

- `U`, `D`, `L`, `R`, `S`

## Hardware Notes

- USART1: PA9 TX, PA10 RX, 115200 8N1.
- LCD: ILI9341/ST7789V over FSMC 8080 interface, following the Wildfire Guide example wiring.
- LED/BEEP: LED_R is PB5, BEEP is PA8, matching the Wildfire Guide examples.
- On-board keys: KEY1/PA0 starts the game or turns left; KEY2/PC13 starts the game or turns right. The PC arrow controller still works.

If the LCD stays white or shows noise, first confirm that the board is the FSMC LCD version and that the Keil target matches the actual MCU package.
