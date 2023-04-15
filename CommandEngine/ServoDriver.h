#ifndef _SERVO_DRIVER_H
#define _SERVO_DRIVER_H

#include <stdint.h>

// REGISTER ADDRESSES
#define PCA9685_MODE1 0x00      /**< Mode Register 1 */
#define PCA9685_MODE2 0x01      /**< Mode Register 2 */
#define PCA9685_SUBADR1 0x02    /**< I2C-bus subaddress 1 */
#define PCA9685_SUBADR2 0x03    /**< I2C-bus subaddress 2 */
#define PCA9685_SUBADR3 0x04    /**< I2C-bus subaddress 3 */
#define PCA9685_ALLCALLADR 0x05 /**< LED All Call I2C-bus address */
#define PCA9685_LED0_ON_L 0x06  /**< LED0 on tick, low byte*/
#define PCA9685_LED0_ON_H 0x07  /**< LED0 on tick, high byte*/
#define PCA9685_LED0_OFF_L 0x08 /**< LED0 off tick, low byte */
#define PCA9685_LED0_OFF_H 0x09 /**< LED0 off tick, high byte */
// etc all 16:  LED15_OFF_H 0x45
#define PCA9685_ALLLED_ON_L 0xFA  /**< load all the LEDn_ON registers, low */
#define PCA9685_ALLLED_ON_H 0xFB  /**< load all the LEDn_ON registers, high */
#define PCA9685_ALLLED_OFF_L 0xFC /**< load all the LEDn_OFF registers, low */
#define PCA9685_ALLLED_OFF_H 0xFD /**< load all the LEDn_OFF registers,high */
#define PCA9685_PRESCALE 0xFE     /**< Prescaler for PWM output frequency */
#define PCA9685_TESTMODE 0xFF     /**< defines the test mode to be entered */

// MODE1 bits
#define MODE1_ALLCAL 0x01  /**< respond to LED All Call I2C-bus address */
#define MODE1_SUB3 0x02    /**< respond to I2C-bus subaddress 3 */
#define MODE1_SUB2 0x04    /**< respond to I2C-bus subaddress 2 */
#define MODE1_SUB1 0x08    /**< respond to I2C-bus subaddress 1 */
#define MODE1_SLEEP 0x10   /**< Low power mode. Oscillator off */
#define MODE1_AI 0x20      /**< Auto-Increment enabled */
#define MODE1_EXTCLK 0x40  /**< Use EXTCLK pin clock */
#define MODE1_RESTART 0x80 /**< Restart enabled */
// MODE2 bits
#define MODE2_OUTNE_0 0x01 /**< Active LOW output enable input */
#define MODE2_OUTNE_1 \
  0x02                    /**< Active LOW output enable input - high impedience */
#define MODE2_OUTDRV 0x04 /**< totem pole structure vs open-drain */
#define MODE2_OCH 0x08    /**< Outputs change on ACK vs STOP */
#define MODE2_INVRT 0x10  /**< Output logic state inverted */

#define PCA9685_I2C_BUS_CHANNEL 1
#define PCA9685_I2C_ADDRESS 0x40      /**< Default PCA9685 I2C Slave Address */
#define FREQUENCY_OSCILLATOR 25000000 /**< Int. osc. frequency in datasheet */

#define PCA9685_PRESCALE_MIN 3   /**< minimum prescale value */
#define PCA9685_PRESCALE_MAX 255 /**< maximum prescale value */
#define PCA9685_MAX_PWM 4095

// WARNING : Due to voltage differences, specially in Raspi vs Arduino situations,
// you may need to adjust this for your particular servo
#define SG90_MIN 125
#define SG90_MAX 490

#define MOTOR_MAX_SPEED 255
#define SERVO_MAX_ANGLE 180
#define SERVO_COUNT 8

// This is for the writing to controller part. Users should not modify directly.
extern char *servoPositions;

int servoDriverInit(uint8_t prescale);
void servoDriverDeInit(unsigned int fd);
void reset();
void servoDriverSleep();
void wakeup();
void setExtClk(uint8_t prescale);
void setPWMFreq(float freq);
void setOutputMode(uint8_t totempole);
uint8_t getPWM(uint8_t num);
void setPWM(uint8_t num, uint16_t on, uint16_t off);
void setPin(uint8_t num, uint16_t val, uint8_t invert);
uint8_t readPrescale(void);
void setPos(uint8_t servoNum, uint8_t position);

// Write multiple PWM pins at the same time
void servoDriverWriteCommands();

void setOscillatorFrequency(uint32_t freq);
uint32_t getOscillatorFrequency(void);

uint8_t read8(uint8_t addr);
void write8(uint8_t addr, uint8_t d);

#endif
