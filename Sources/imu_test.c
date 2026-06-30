#include <stdint.h>

// RCC
#define RCC_BASE        0x40023800
#define RCC_AHB1ENR     (*((volatile uint32_t *)(RCC_BASE + 0x30)))
#define RCC_APB2ENR     (*((volatile uint32_t *)(RCC_BASE + 0x44)))

// GPIOA
#define GPIOA_BASE      0x40020000
#define GPIOA_MODER     (*((volatile uint32_t *)(GPIOA_BASE + 0x00)))
#define GPIOA_OSPEEDR   (*((volatile uint32_t *)(GPIOA_BASE + 0x08)))
#define GPIOA_ODR       (*((volatile uint32_t *)(GPIOA_BASE + 0x14)))
#define GPIOA_AFRL      (*((volatile uint32_t *)(GPIOA_BASE + 0x20)))

// SPI1
#define SPI1_BASE       0x40013000
#define SPI1_CR1        (*((volatile uint32_t *)(SPI1_BASE + 0x00)))
#define SPI1_SR         (*((volatile uint32_t *)(SPI1_BASE + 0x08)))
#define SPI1_DR         (*((volatile uint32_t *)(SPI1_BASE + 0x0C)))

#define SPI1_SR_TXE     (1 << 1)
#define SPI1_SR_RXNE    (1 << 0)

#define CS_LOW()   (GPIOA_ODR &= ~(1 << 4))
#define CS_HIGH()  (GPIOA_ODR |=  (1 << 4))

// LSM6DSOX WHO_AM_I
#define WHO_AM_I_REG      0x0F
#define WHO_AM_I_EXPECTED 0x6C

void SPI1_GPIO_Init(void) {
    RCC_AHB1ENR |= (1 << 0);  // enable GPIOA clock

    // PA5, PA6, PA7 -> alternate function mode (10)
    GPIOA_MODER &= ~((3<<10) | (3<<12) | (3<<14));
    GPIOA_MODER |=  ((2<<10) | (2<<12) | (2<<14));

    // AF5 for SPI1 on PA5/PA6/PA7
    GPIOA_AFRL &= ~((0xF<<20) | (0xF<<24) | (0xF<<28));
    GPIOA_AFRL |=  ((5<<20)  | (5<<24)  | (5<<28));

    GPIOA_OSPEEDR |= (3<<10) | (3<<12) | (3<<14);

    // PA4 -> CS, general purpose output (01)
    GPIOA_MODER &= ~(3<<8);
    GPIOA_MODER |=  (1<<8);
    CS_HIGH();
}

void SPI1_Init(void) {
    RCC_APB2ENR |= (1 << 12); // enable SPI1 clock

    SPI1_CR1 = 0;
    SPI1_CR1 |= (1 << 2);          // MSTR
    SPI1_CR1 |= (3 << 3);          // baud rate prescaler /16
    SPI1_CR1 |= (1 << 8) | (1 << 9); // SSM | SSI
    SPI1_CR1 |= (1 << 6);          // SPE
}

uint8_t SPI1_Transfer(uint8_t data) {
    while (!(SPI1_SR & SPI1_SR_TXE));
    SPI1_DR = data;
    while (!(SPI1_SR & SPI1_SR_RXNE));
    return SPI1_DR;
}

uint8_t LSM6DSOX_ReadReg(uint8_t reg) {
    uint8_t value;
    CS_LOW();
    SPI1_Transfer(reg | 0x80);   // MSB high = read
    value = SPI1_Transfer(0x00);
    CS_HIGH();
    return value;
}

void delay(volatile uint32_t count) {
    while (count--);
}

int main(void) {
    SPI1_GPIO_Init();
    SPI1_Init();

    // Pull CS low->high once to latch the chip into SPI mode
    CS_LOW();
    delay(1000);
    CS_HIGH();
    delay(1000);

    delay(1000000);   // let the IMU power up and settle

    uint8_t who = LSM6DSOX_ReadReg(WHO_AM_I_REG);
    (void)who;        // breakpoint here; expect who == 0x6C

    while (1) {
        __asm("nop");
    }
}
