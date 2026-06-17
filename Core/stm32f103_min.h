#ifndef STM32F103_MIN_H
#define STM32F103_MIN_H

#include <stdint.h>

#define __IO volatile

#define PERIPH_BASE        0x40000000UL
#define APB1PERIPH_BASE    PERIPH_BASE
#define APB2PERIPH_BASE    (PERIPH_BASE + 0x00010000UL)
#define AHBPERIPH_BASE     (PERIPH_BASE + 0x00018000UL)

#define FLASH_R_BASE       0x40022000UL
#define RCC_BASE           (AHBPERIPH_BASE + 0x00009000UL)
#define FSMC_R_BASE        0xA0000000UL
#define GPIOA_BASE         (APB2PERIPH_BASE + 0x00000800UL)
#define GPIOB_BASE         (APB2PERIPH_BASE + 0x00000C00UL)
#define GPIOC_BASE         (APB2PERIPH_BASE + 0x00001000UL)
#define GPIOD_BASE         (APB2PERIPH_BASE + 0x00001400UL)
#define GPIOE_BASE         (APB2PERIPH_BASE + 0x00001800UL)
#define USART1_BASE        (APB2PERIPH_BASE + 0x00003800UL)

#define SCS_BASE           0xE000E000UL
#define SYSTICK_BASE       (SCS_BASE + 0x0010UL)
#define NVIC_ISER0         (*((__IO uint32_t *)0xE000E100UL))
#define NVIC_ISER1         (*((__IO uint32_t *)0xE000E104UL))
#define NVIC_IPR_BASE      ((volatile uint8_t *)0xE000E400UL)

typedef struct {
    __IO uint32_t CRL;
    __IO uint32_t CRH;
    __IO uint32_t IDR;
    __IO uint32_t ODR;
    __IO uint32_t BSRR;
    __IO uint32_t BRR;
    __IO uint32_t LCKR;
} GPIO_TypeDef;

typedef struct {
    __IO uint32_t CR;
    __IO uint32_t CFGR;
    __IO uint32_t CIR;
    __IO uint32_t APB2RSTR;
    __IO uint32_t APB1RSTR;
    __IO uint32_t AHBENR;
    __IO uint32_t APB2ENR;
    __IO uint32_t APB1ENR;
    __IO uint32_t BDCR;
    __IO uint32_t CSR;
} RCC_TypeDef;

typedef struct {
    __IO uint32_t ACR;
    __IO uint32_t KEYR;
    __IO uint32_t OPTKEYR;
    __IO uint32_t SR;
    __IO uint32_t CR;
    __IO uint32_t AR;
    __IO uint32_t RESERVED;
    __IO uint32_t OBR;
    __IO uint32_t WRPR;
} FLASH_TypeDef;

typedef struct {
    __IO uint32_t SR;
    __IO uint32_t DR;
    __IO uint32_t BRR;
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t CR3;
    __IO uint32_t GTPR;
} USART_TypeDef;

typedef struct {
    __IO uint32_t BTCR[8];
} FSMC_Bank1_TypeDef;

typedef struct {
    __IO uint32_t BWTR[7];
} FSMC_Bank1E_TypeDef;

typedef struct {
    __IO uint32_t CTRL;
    __IO uint32_t LOAD;
    __IO uint32_t VAL;
    __IO uint32_t CALIB;
} SysTick_TypeDef;

#define GPIOA              ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB              ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC              ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD              ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE              ((GPIO_TypeDef *)GPIOE_BASE)
#define RCC                ((RCC_TypeDef *)RCC_BASE)
#define FLASH              ((FLASH_TypeDef *)FLASH_R_BASE)
#define USART1             ((USART_TypeDef *)USART1_BASE)
#define FSMC_Bank1         ((FSMC_Bank1_TypeDef *)FSMC_R_BASE)
#define FSMC_Bank1E        ((FSMC_Bank1E_TypeDef *)(FSMC_R_BASE + 0x0104UL))
#define SysTick            ((SysTick_TypeDef *)SYSTICK_BASE)

#define RCC_CR_HSION       (1UL << 0)
#define RCC_CR_HSEON       (1UL << 16)
#define RCC_CR_HSERDY      (1UL << 17)
#define RCC_CR_PLLON       (1UL << 24)
#define RCC_CR_PLLRDY      (1UL << 25)

#define RCC_CFGR_SW_PLL    0x00000002UL
#define RCC_CFGR_SWS_PLL   0x00000008UL
#define RCC_CFGR_HPRE_DIV1 0x00000000UL
#define RCC_CFGR_PPRE1_DIV2 0x00000400UL
#define RCC_CFGR_PPRE2_DIV1 0x00000000UL
#define RCC_CFGR_PLLSRC    (1UL << 16)
#define RCC_CFGR_PLLMULL9  (7UL << 18)

#define RCC_APB2ENR_AFIOEN   (1UL << 0)
#define RCC_APB2ENR_IOPAEN   (1UL << 2)
#define RCC_APB2ENR_IOPBEN   (1UL << 3)
#define RCC_APB2ENR_IOPCEN   (1UL << 4)
#define RCC_APB2ENR_IOPDEN   (1UL << 5)
#define RCC_APB2ENR_IOPEEN   (1UL << 6)
#define RCC_APB2ENR_USART1EN (1UL << 14)
#define RCC_AHBENR_FSMCEN    (1UL << 8)

#define FLASH_ACR_LATENCY_2  0x00000002UL
#define FLASH_ACR_PRFTBE     0x00000010UL
#define FLASH_SR_BSY         (1UL << 0)
#define FLASH_SR_EOP         (1UL << 5)
#define FLASH_CR_PG          (1UL << 0)
#define FLASH_CR_PER         (1UL << 1)
#define FLASH_CR_STRT        (1UL << 6)
#define FLASH_CR_LOCK        (1UL << 7)

#define USART_SR_RXNE        (1UL << 5)
#define USART_SR_TXE         (1UL << 7)
#define USART_CR1_RE         (1UL << 2)
#define USART_CR1_TE         (1UL << 3)
#define USART_CR1_RXNEIE     (1UL << 5)
#define USART_CR1_UE         (1UL << 13)

#define SYSTICK_CTRL_ENABLE  (1UL << 0)
#define SYSTICK_CTRL_TICKINT (1UL << 1)
#define SYSTICK_CTRL_CLKSRC  (1UL << 2)
#define SYSTICK_CTRL_COUNT   (1UL << 16)

#define USART1_IRQn          37

extern uint32_t SystemCoreClock;

void SystemInit(void);
void delay_init(void);
void delay_ms(uint32_t ms);
uint32_t millis(void);
void nvic_enable_irq(uint32_t irq, uint8_t priority);

#endif
