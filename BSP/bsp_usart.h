#ifndef BSP_USART_H
#define BSP_USART_H

void usart1_init(void);
void usart1_send_char(char ch);
void usart1_send_string(const char *str);

#endif
