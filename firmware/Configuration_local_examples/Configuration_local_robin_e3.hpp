/**
 * @brief Local hardware configuration for the OpenAstroExplorer on the MKS Robin E3 v1.1
 *        (STM32F103RC, on-board TMC2209 drivers, one single-wire UART pin each).
 *
 * LocalConfiguration.hpp falls back to this committed file when no
 * Configuration_local_robin_e3.hpp exists in firmware/. Copy it there to customise.
 * Pins: boards/STM32_MKS_ROBIN_E3/pins_MKS_ROBIN_E3.hpp. R_SENSE (0.11 ohm) is set in
 * src/a_inits.hpp.
 */
#pragma once

#define BOARD BOARD_STM32_MKS_ROBIN_E3

#define OAE
#define NORTHERN_HEMISPHERE 1

// No display, GPS or gyro on this board.
#define DISPLAY_TYPE      DISPLAY_TYPE_NONE
#define INFO_DISPLAY_TYPE INFO_DISPLAY_TYPE_NONE
#define USE_GPS           0
#define USE_GYRO_LEVEL    0

#define RA_STEPPER_TYPE    STEPPER_TYPE_ENABLED
#define DEC_STEPPER_TYPE   STEPPER_TYPE_ENABLED
#define AZ_STEPPER_TYPE    STEPPER_TYPE_ENABLED
#define ALT_STEPPER_TYPE   STEPPER_TYPE_NONE
#define FOCUS_STEPPER_TYPE STEPPER_TYPE_ENABLED

#define RA_DRIVER_TYPE  DRIVER_TYPE_TMC2209_UART
#define DEC_DRIVER_TYPE DRIVER_TYPE_TMC2209_UART
// AZ: bipolar-modded 28BYJ-48 on the Z socket (step PC14, dir PC15, EN PB8, UART PC12). ALT must stay off, it shares the
// socket. For the stock ULN2003 board instead, see docs/wiring.md.
#define AZ_DRIVER_TYPE DRIVER_TYPE_TMC2209_UART
// Focus: bipolar-modded 28BYJ-48 on the E socket (step PB4, dir PB3, EN PB5, UART PC11), same motor rules as AZ.
#define FOCUS_DRIVER_TYPE DRIVER_TYPE_TMC2209_UART

// Motors rated at 470 mA, run at 75 % (RMS computed by Configuration_adv.hpp).
#define RA_MOTOR_CURRENT_RATING      470  // mA
#define RA_OPERATING_CURRENT_SETTING 75   // %

#define DEC_MOTOR_CURRENT_RATING      470  // mA
#define DEC_OPERATING_CURRENT_SETTING 75   // %

// AZ: 5 V 28BYJ-48 with the centre tap cut (bipolar, two halves in series, R_full = 2 * R_half, typically R_half 22-50 ohm).
// The driver chops the current, so the 12/24 V supply is fine once the UART current is applied. Never run it on the
// board's VREF current. Rated half-coil current is 5 V / R_half. Series coils need about 0.5x of it for equal torque and
// 0.7x for equal heat. For intermittent duty: Irms = 0.7 * 5 / R_half (R_half 25 ohm -> 140 mA, 50 ohm -> 70 mA).
// AZ_MOTOR_CURRENT_RATING is a PEAK value: Configuration_adv.hpp divides by 1.414 to get RMS.
// 150 mA peak at 100 % = 106 mA RMS requested. The TMC2209 library (R_SENSE 0.11 ohm + 0.02, vsense on) rounds that down to
// IRUN 2 of 31, which is 91 mA RMS. IRUN 3 would be 122 mA, so any request from 92 to 122 mA gives the same 91 mA.
#define AZ_MOTOR_CURRENT_RATING      150  // mA, peak
#define AZ_OPERATING_CURRENT_SETTING 100  // %
// Motor hold current in % of run current. Low: with AZ_ALWAYS_ON 0 the driver is released (EN high) when idle anyway.
#define AZ_MOTOR_HOLD_SETTING 30
#define AZ_ALWAYS_ON         0
// Full bipolar coil (both halves in series) for the stealthChop PWM_OFS start value, ohms. Measure it across one coil pair.
#define AZ_COIL_RESISTANCE_OHM 50.0f

// 28BYJ-48: 32 full steps x ~63.68:1 gearbox = ~2038 steps per output turn (nominal 64:1 gives 2048). 16 microsteps.
// AZIMUTH_STEPS_PER_REV (OAE) = AZ_CORRECTION_FACTOR * (AZ_CIRCUMFERENCE / AZ_ROD_PITCH) * AZ_STEPPER_SPR * AZ_MICROSTEPPING,
// with the OAE 56 mm radius and 0.5 mm rod pitch: 703.7 rod turns * 2048 * 16 = 23.06 Mi microsteps per azimuth turn.
// The real gearbox is 0.5 % lower than 2048: calibrate with AZ_CORRECTION_FACTOR (about 0.995) if it matters.
// Speed and acceleration are in microsteps/s: 400 full steps/s (the motor tops out at 500-800) and 200 full steps/s^2.
#define AZ_STEPPER_SPR          2048
#define AZ_MICROSTEPPING        16
#define AZ_STEPPER_SPEED        (400 * AZ_MICROSTEPPING)
#define AZ_STEPPER_ACCELERATION (200 * AZ_MICROSTEPPING)

// Focus: same motor and current rule as AZ (150 mA peak = 91 mA RMS), hold low, released when idle (FOCUSER_ALWAYS_ON 0).
// Speed and acceleration in microsteps/s, like AZ.
#define FOCUS_MOTOR_CURRENT_RATING      150  // mA, peak
#define FOCUS_OPERATING_CURRENT_SETTING 100  // %
#define FOCUSER_MOTOR_HOLD_SETTING      30
#define FOCUSER_ALWAYS_ON               0
#define FOCUS_STEPPER_SPR               2048
#define FOCUS_MICROSTEPPING             16
#define FOCUS_STEPPER_SPEED             (400 * FOCUS_MICROSTEPPING)
#define FOCUS_STEPPER_ACCELERATION      (200 * FOCUS_MICROSTEPPING)

// A driver that does not answer on its UART is disabled (EN high) instead of running at the board's VREF current.
#define UART_CONNECTION_TEST_TXRX 1

#define TRACK_ON_BOOT 0

// Motor supply, 12 or 24 V.
#define SUPPLY_VOLTAGE 24

// Ender-3 LCD and knob on the Ender-3 header.
#define USE_LCD_UI 1

// The stock Ender-3 LCD knob turns the wrong way without this.
#define ENCODER_INVERT 1
