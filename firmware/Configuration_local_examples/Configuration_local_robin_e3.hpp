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
#define AZ_STEPPER_TYPE    STEPPER_TYPE_NONE
#define ALT_STEPPER_TYPE   STEPPER_TYPE_NONE
#define FOCUS_STEPPER_TYPE STEPPER_TYPE_NONE

#define RA_DRIVER_TYPE  DRIVER_TYPE_TMC2209_UART
#define DEC_DRIVER_TYPE DRIVER_TYPE_TMC2209_UART

// Motors rated at 470 mA, run at 75 % (RMS computed by Configuration_adv.hpp).
#define RA_MOTOR_CURRENT_RATING      470  // mA
#define RA_OPERATING_CURRENT_SETTING 75   // %

#define DEC_MOTOR_CURRENT_RATING      470  // mA
#define DEC_OPERATING_CURRENT_SETTING 75   // %

// A driver that does not answer on its UART is disabled (EN high) instead of running at the board's VREF current.
#define UART_CONNECTION_TEST_TXRX 1

#define TRACK_ON_BOOT 0
