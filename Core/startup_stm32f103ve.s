Stack_Size      EQU     0x00001000

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
__Vectors       DCD     __initial_sp
                DCD     Reset_Handler
                DCD     NMI_Handler
                DCD     HardFault_Handler
                DCD     MemManage_Handler
                DCD     BusFault_Handler
                DCD     UsageFault_Handler
                DCD     0, 0, 0, 0
                DCD     SVC_Handler
                DCD     DebugMon_Handler
                DCD     0
                DCD     PendSV_Handler
                DCD     SysTick_Handler
                DCD     WWDG_IRQHandler
                DCD     PVD_IRQHandler
                DCD     TAMPER_IRQHandler
                DCD     RTC_IRQHandler
                DCD     FLASH_IRQHandler
                DCD     RCC_IRQHandler
                DCD     EXTI0_IRQHandler
                DCD     EXTI1_IRQHandler
                DCD     EXTI2_IRQHandler
                DCD     EXTI3_IRQHandler
                DCD     EXTI4_IRQHandler
                DCD     DMA1_Channel1_IRQHandler
                DCD     DMA1_Channel2_IRQHandler
                DCD     DMA1_Channel3_IRQHandler
                DCD     DMA1_Channel4_IRQHandler
                DCD     DMA1_Channel5_IRQHandler
                DCD     DMA1_Channel6_IRQHandler
                DCD     DMA1_Channel7_IRQHandler
                DCD     ADC1_2_IRQHandler
                DCD     USB_HP_CAN1_TX_IRQHandler
                DCD     USB_LP_CAN1_RX0_IRQHandler
                DCD     CAN1_RX1_IRQHandler
                DCD     CAN1_SCE_IRQHandler
                DCD     EXTI9_5_IRQHandler
                DCD     TIM1_BRK_IRQHandler
                DCD     TIM1_UP_IRQHandler
                DCD     TIM1_TRG_COM_IRQHandler
                DCD     TIM1_CC_IRQHandler
                DCD     TIM2_IRQHandler
                DCD     TIM3_IRQHandler
                DCD     TIM4_IRQHandler
                DCD     I2C1_EV_IRQHandler
                DCD     I2C1_ER_IRQHandler
                DCD     I2C2_EV_IRQHandler
                DCD     I2C2_ER_IRQHandler
                DCD     SPI1_IRQHandler
                DCD     SPI2_IRQHandler
                DCD     USART1_IRQHandler

                AREA    |.text|, CODE, READONLY

Reset_Handler   PROC
                EXPORT  Reset_Handler
                IMPORT  SystemInit
                IMPORT  main
                BL      SystemInit
                B       main
                ENDP

Default_Handler PROC
                EXPORT  NMI_Handler              [WEAK]
                EXPORT  HardFault_Handler        [WEAK]
                EXPORT  MemManage_Handler        [WEAK]
                EXPORT  BusFault_Handler         [WEAK]
                EXPORT  UsageFault_Handler       [WEAK]
                EXPORT  SVC_Handler              [WEAK]
                EXPORT  DebugMon_Handler         [WEAK]
                EXPORT  PendSV_Handler           [WEAK]
                EXPORT  SysTick_Handler          [WEAK]
                EXPORT  WWDG_IRQHandler          [WEAK]
                EXPORT  PVD_IRQHandler           [WEAK]
                EXPORT  TAMPER_IRQHandler        [WEAK]
                EXPORT  RTC_IRQHandler           [WEAK]
                EXPORT  FLASH_IRQHandler         [WEAK]
                EXPORT  RCC_IRQHandler           [WEAK]
                EXPORT  EXTI0_IRQHandler         [WEAK]
                EXPORT  EXTI1_IRQHandler         [WEAK]
                EXPORT  EXTI2_IRQHandler         [WEAK]
                EXPORT  EXTI3_IRQHandler         [WEAK]
                EXPORT  EXTI4_IRQHandler         [WEAK]
                EXPORT  DMA1_Channel1_IRQHandler [WEAK]
                EXPORT  DMA1_Channel2_IRQHandler [WEAK]
                EXPORT  DMA1_Channel3_IRQHandler [WEAK]
                EXPORT  DMA1_Channel4_IRQHandler [WEAK]
                EXPORT  DMA1_Channel5_IRQHandler [WEAK]
                EXPORT  DMA1_Channel6_IRQHandler [WEAK]
                EXPORT  DMA1_Channel7_IRQHandler [WEAK]
                EXPORT  ADC1_2_IRQHandler        [WEAK]
                EXPORT  USB_HP_CAN1_TX_IRQHandler [WEAK]
                EXPORT  USB_LP_CAN1_RX0_IRQHandler [WEAK]
                EXPORT  CAN1_RX1_IRQHandler      [WEAK]
                EXPORT  CAN1_SCE_IRQHandler      [WEAK]
                EXPORT  EXTI9_5_IRQHandler       [WEAK]
                EXPORT  TIM1_BRK_IRQHandler      [WEAK]
                EXPORT  TIM1_UP_IRQHandler       [WEAK]
                EXPORT  TIM1_TRG_COM_IRQHandler  [WEAK]
                EXPORT  TIM1_CC_IRQHandler       [WEAK]
                EXPORT  TIM2_IRQHandler          [WEAK]
                EXPORT  TIM3_IRQHandler          [WEAK]
                EXPORT  TIM4_IRQHandler          [WEAK]
                EXPORT  I2C1_EV_IRQHandler       [WEAK]
                EXPORT  I2C1_ER_IRQHandler       [WEAK]
                EXPORT  I2C2_EV_IRQHandler       [WEAK]
                EXPORT  I2C2_ER_IRQHandler       [WEAK]
                EXPORT  SPI1_IRQHandler          [WEAK]
                EXPORT  SPI2_IRQHandler          [WEAK]
                EXPORT  USART1_IRQHandler        [WEAK]
NMI_Handler
HardFault_Handler
MemManage_Handler
BusFault_Handler
UsageFault_Handler
SVC_Handler
DebugMon_Handler
PendSV_Handler
SysTick_Handler
WWDG_IRQHandler
PVD_IRQHandler
TAMPER_IRQHandler
RTC_IRQHandler
FLASH_IRQHandler
RCC_IRQHandler
EXTI0_IRQHandler
EXTI1_IRQHandler
EXTI2_IRQHandler
EXTI3_IRQHandler
EXTI4_IRQHandler
DMA1_Channel1_IRQHandler
DMA1_Channel2_IRQHandler
DMA1_Channel3_IRQHandler
DMA1_Channel4_IRQHandler
DMA1_Channel5_IRQHandler
DMA1_Channel6_IRQHandler
DMA1_Channel7_IRQHandler
ADC1_2_IRQHandler
USB_HP_CAN1_TX_IRQHandler
USB_LP_CAN1_RX0_IRQHandler
CAN1_RX1_IRQHandler
CAN1_SCE_IRQHandler
EXTI9_5_IRQHandler
TIM1_BRK_IRQHandler
TIM1_UP_IRQHandler
TIM1_TRG_COM_IRQHandler
TIM1_CC_IRQHandler
TIM2_IRQHandler
TIM3_IRQHandler
TIM4_IRQHandler
I2C1_EV_IRQHandler
I2C1_ER_IRQHandler
I2C2_EV_IRQHandler
I2C2_ER_IRQHandler
SPI1_IRQHandler
SPI2_IRQHandler
USART1_IRQHandler
                B       .
                ENDP

                END
