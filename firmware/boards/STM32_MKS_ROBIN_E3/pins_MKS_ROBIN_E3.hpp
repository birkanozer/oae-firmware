/**
 * @brief Pin configuration for the MKS Robin E3 v1.1 (STM32F103RCT6, TMC2209 on board).
 *
 * Single source for the wiring: docs/wiring.md.
 *
 * Stepper socket mapping (OAT axis -> socket):
 *   X -> RA, Y -> DEC, Z -> ALT (free socket), E -> FOCUS (free socket)
 *
 * Each TMC2209 has its own single-wire UART (PDN) pin, driven by SoftwareSerial on TIM7
 * (-D TIMER_SERIAL=TIM7). TX and RX are the same pin; address 0 on every driver.
 *
 * Not assigned here (reserved for the screen): PB10, PB11, PC10, PB13, PA15, PB0.
 */

#pragma once

// ── RA axis (X socket) ───────────────────────────────────────────────────────
#ifndef RA_STEP_PIN
    #define RA_STEP_PIN PC0
#endif
#ifndef RA_DIR_PIN
    #define RA_DIR_PIN PB2
#endif
#ifndef RA_EN_PIN
    #define RA_EN_PIN PC13  // active LOW
#endif
#ifndef RA_DIAG_PIN
    #define RA_DIAG_PIN PA12  // X- endstop
#endif
#ifndef RA_SERIAL_PORT_TX
    #define RA_SERIAL_PORT_TX PC7  // PDN, single wire: RX == TX
#endif
#ifndef RA_SERIAL_PORT_RX
    #define RA_SERIAL_PORT_RX PC7
#endif
#ifndef RA_DRIVER_ADDRESS
    #define RA_DRIVER_ADDRESS 0b00
#endif

// ── DEC axis (Y socket) ──────────────────────────────────────────────────────
#ifndef DEC_STEP_PIN
    #define DEC_STEP_PIN PC2
#endif
#ifndef DEC_DIR_PIN
    #define DEC_DIR_PIN PB9
#endif
#ifndef DEC_EN_PIN
    #define DEC_EN_PIN PB12  // active LOW
#endif
#ifndef DEC_DIAG_PIN
    #define DEC_DIAG_PIN PA11  // Y- endstop
#endif
#ifndef DEC_SERIAL_PORT_TX
    #define DEC_SERIAL_PORT_TX PD2
#endif
#ifndef DEC_SERIAL_PORT_RX
    #define DEC_SERIAL_PORT_RX PD2
#endif
#ifndef DEC_DRIVER_ADDRESS
    #define DEC_DRIVER_ADDRESS 0b00
#endif

// ── ALT axis (Z socket; AZ homing endstop Z- is PC6, spare Z+ is PB1) ────────
#ifndef ALT_STEP_PIN
    #define ALT_STEP_PIN PC14
#endif
#ifndef ALT_DIR_PIN
    #define ALT_DIR_PIN PC15
#endif
#ifndef ALT_EN_PIN
    #define ALT_EN_PIN PB8  // active LOW
#endif
#ifndef ALT_DIAG_PIN
    #define ALT_DIAG_PIN PB1  // Z+ / probe endstop
#endif
#ifndef ALT_SERIAL_PORT_TX
    #define ALT_SERIAL_PORT_TX PC12
#endif
#ifndef ALT_SERIAL_PORT_RX
    #define ALT_SERIAL_PORT_RX PC12
#endif
#ifndef ALT_DRIVER_ADDRESS
    #define ALT_DRIVER_ADDRESS 0b00
#endif
#ifndef AZ_DIAG_PIN
    #define AZ_DIAG_PIN PC6  // Z- endstop
#endif

// ── FOCUS axis (E socket; PB3/PB4 need JTAG off, done first thing in setup()) ─
#ifndef FOCUS_STEP_PIN
    #define FOCUS_STEP_PIN PB4
#endif
#ifndef FOCUS_DIR_PIN
    #define FOCUS_DIR_PIN PB3
#endif
#ifndef FOCUS_EN_PIN
    #define FOCUS_EN_PIN PB5  // active LOW
#endif
#ifndef FOCUS_SERIAL_PORT_TX
    #define FOCUS_SERIAL_PORT_TX PC11
#endif
#ifndef FOCUS_SERIAL_PORT_RX
    #define FOCUS_SERIAL_PORT_RX PC11
#endif
#ifndef FOCUS_DRIVER_ADDRESS
    #define FOCUS_DRIVER_ADDRESS 0b00
#endif

// All drivers use software serial (one pin each).
#define SW_SERIAL_UART 1

// ── I2C1 (EEPROM, later) ─────────────────────────────────────────────────────
#ifndef I2C_SCL_PIN
    #define I2C_SCL_PIN PB6
#endif
#ifndef I2C_SDA_PIN
    #define I2C_SDA_PIN PB7
#endif

// ── Serial ports ─────────────────────────────────────────────────────────────
// Serial = USART1 on USB (SERIAL_BAUDRATE). Serial2 = USART2 PA2 TX / PA3 RX to the bridge.
#ifndef DEBUG_SERIAL_PORT
    #define DEBUG_SERIAL_PORT Serial
#endif
#ifndef BRIDGE_SERIAL_PORT
    #define BRIDGE_SERIAL_PORT Serial2
#endif
#ifndef BRIDGE_SERIAL_BAUDRATE
    #define BRIDGE_SERIAL_BAUDRATE 115200
#endif

// ── Stock OAE mechanics (same as boards/ESP32_ESP32DEV/pins_OAE_V1.hpp) ──────
#ifndef RA_WHEEL_CIRCUMFERENCE
    #define RA_WHEEL_CIRCUMFERENCE 704.97f
#endif
#ifndef DEC_TRANSMISSION
    #define DEC_TRANSMISSION (DEC_WHEEL_CIRCUMFERENCE / (DEC_PULLEY_TEETH * 1.0))
#endif
#ifndef RA_LIMIT_LEFT
    #define RA_LIMIT_LEFT 5.0f
#endif
#ifndef RA_LIMIT_RIGHT
    #define RA_LIMIT_RIGHT 7.0f
#endif
#ifndef RA_TRACKING_LIMIT
    #define RA_TRACKING_LIMIT 6.75f
#endif
#ifndef DEC_WHEEL_CIRCUMFERENCE
    #define DEC_WHEEL_CIRCUMFERENCE 1.0f
#endif
#ifndef RA_STEPPER_SPR
    #define RA_STEPPER_SPR (400 * 9)  // change to (200 * 9) for 1.8° stepper
#endif
#ifndef DEC_STEPPER_SPR
    #define DEC_STEPPER_SPR (200 * 50 * 4.5f)  // change to (200 * 9) for 1.8° stepper
#endif
#ifndef DEC_PULLEY_TEETH
    #define DEC_PULLEY_TEETH 1
#endif
