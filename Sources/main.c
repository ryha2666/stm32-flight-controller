#include <stdint.h>

// RCC and GPIOC register base addresses for STM32F411
#define RCC_BASE    0x40023800
#define GPIOC_BASE  0x40020800

#define RCC_AHB1ENR  (*((volatile uint32_t *)(RCC_BASE + 0x30)))
#define GPIOC_MODER  (*((volatile uint32_t *)(GPIOC_BASE + 0x00)))
#define GPIOC_ODR    (*((volatile uint32_t *)(GPIOC_BASE + 0x14)))

void delay(volatile uint32_t count) {
    while(count--);
}

int main(void) {
    // Enable GPIOC clock
    RCC_AHB1ENR |= (1 << 2);

    // Set PC13 as output (MODER bits 27:26 = 01)
    GPIOC_MODER &= ~(3 << 26);
    GPIOC_MODER |=  (1 << 26);

    while(1) {
        GPIOC_ODR |=  (1 << 13);  // PC13 high (LED off)
        delay(1000000);
        GPIOC_ODR &= ~(1 << 13);  // PC13 low (LED on)
        delay(1000000);
    }
}
