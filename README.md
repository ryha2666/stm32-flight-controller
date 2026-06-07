# STM32F411 Custom Quadcopter Flight Controller

A custom flight controller built from scratch on an STM32F411 Black Pill microcontroller, targeting GNC and avionics applications. No off-the-shelf firmware — all control logic written in C from bare metal.

## Why

Most drone builds use Betaflight or similar pre-built firmware. This project implements the full stack from scratch: IMU communication, sensor fusion, and cascaded PID control loops — demonstrating real embedded systems and control theory work.

## Hardware

| Component | Part |
|-----------|------|
| Microcontroller | STM32F411CEU6 (Black Pill) |
| IMU | ICM-42688-P (SPI) |
| Frame | 3" carbon fiber, 135mm wheelbase |
| Motors | iFlight XING2 1404 3800KV |
| ESC | Skystars KO25 25A 4-in-1 BLHeli_S |
| Battery | Tattu 650mAh 4S LiPo |

## Build Phases

- [x] Phase 1 — Toolchain setup, bare-metal LED blink
- [ ] Phase 2 — ICM-42688 IMU over SPI
- [ ] Phase 3 — Madgwick filter for attitude estimation
- [ ] Phase 4 — Cascaded PID loops (roll, pitch, yaw)
- [ ] Phase 5 — Test stand validation, PID response plots

## Tools

- STM32CubeIDE
- ST-Link V2 (SWD)
- C (bare metal, no HAL)
