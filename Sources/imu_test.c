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

// LSM6DSOX registers
#define WHO_AM_I_REG      0x0F
#define WHO_AM_I_EXPECTED 0x6C
#define CTRL1_XL          0x10   // accelerometer config
#define CTRL2_G           0x11   // gyroscope config
#define OUTX_L_G          0x22   // gyro  X low byte (output block start)
#define OUTX_L_A          0x28   // accel X low byte (output block start)

// Config bytes
// CTRL1_XL = 0x40 -> ODR 104 Hz, full-scale +/-2g  (FS bits 00)
// CTRL2_G  = 0x44 -> ODR 104 Hz, full-scale 500 dps
#define CTRL1_XL_CFG      0x40
#define CTRL2_G_CFG       0x44

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

void LSM6DSOX_WriteReg(uint8_t reg, uint8_t data) {
    CS_LOW();
    SPI1_Transfer(reg & 0x7F);   // MSB low = write
    SPI1_Transfer(data);
    CS_HIGH();
}

// Read a 6-byte output block starting at start_reg (auto-increments),
// combine into signed 16-bit x/y/z. Little-endian: low byte first.
void LSM6DSOX_ReadBlock(uint8_t start_reg, int16_t *x, int16_t *y, int16_t *z) {
    uint8_t raw[6];
    CS_LOW();
    SPI1_Transfer(start_reg | 0x80);
    for (int i = 0; i < 6; i++) {
        raw[i] = SPI1_Transfer(0x00);
    }
    CS_HIGH();

    *x = (int16_t)((raw[1] << 8) | raw[0]);
    *y = (int16_t)((raw[3] << 8) | raw[2]);
    *z = (int16_t)((raw[5] << 8) | raw[4]);
}

void LSM6DSOX_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az) {
    LSM6DSOX_ReadBlock(OUTX_L_A, ax, ay, az);
}

void LSM6DSOX_ReadGyro(int16_t *gx, int16_t *gy, int16_t *gz) {
    LSM6DSOX_ReadBlock(OUTX_L_G, gx, gy, gz);
}

void delay(volatile uint32_t count) {
    while (count--);
}

int main(void) {
    SPI1_GPIO_Init();
    SPI1_Init();

    // Latch SPI mode
    CS_LOW();
    delay(1000);
    CS_HIGH();
    delay(1000);

    delay(1000000);   // let the IMU power up and settle

    // Sanity check
    uint8_t who = LSM6DSOX_ReadReg(WHO_AM_I_REG);
    (void)who;        // expect 0x6C

    // Wake both sensors out of power-down
    LSM6DSOX_WriteReg(CTRL1_XL, CTRL1_XL_CFG);   // accel: 104 Hz, +/-2g
    LSM6DSOX_WriteReg(CTRL2_G,  CTRL2_G_CFG);    // gyro:  104 Hz, 500 dps

    delay(100000);    // let the first samples come ready

    int16_t ax = 0, ay = 0, az = 0;
    int16_t gx = 0, gy = 0, gz = 0;

    LSM6DSOX_ReadAccel(&ax, &ay, &az);
    LSM6DSOX_ReadGyro(&gx, &gy, &gz);
    (void)ax; (void)ay; (void)az;
    (void)gx; (void)gy; (void)gz;   // breakpoint here

    while (1) {
        __asm("nop");
    }
}
