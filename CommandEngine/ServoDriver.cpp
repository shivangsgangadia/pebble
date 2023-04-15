#include "ServoDriver.h"
#include <stdio.h>
#include <unistd.h> // For C file functions
#ifndef TEST_MODE
#include <pigpio.h>
#endif
#include <cstring>
#include <iostream>

using namespace std;

#ifndef TEST_MODE

int controllerFileDescriptor = -1;
uint32_t _oscillator_freq = FREQUENCY_OSCILLATOR;
//uint8_t *servoPositions;

/*!
 *  @brief  Setups the I2C interface and hardware
 *  @param  prescale
 *          Sets External Clock (Optional)
 */
int servoDriverInit(uint8_t prescale) {
  // Init servo position array
  servoPositions = new char[SERVO_COUNT];
  for (int i = 0; i < SERVO_COUNT; i++) {
	  servoPositions[i] = 1;
  }
  //memset(servoPositions, 0, sizeof(uint8_t) * SERVO_COUNT);
  
  // Initialize GPIO library
  if (gpioInitialise() < 0) {
    perror("Unable to init GPIO library");
  }
  
  prescale = 0;
  // Load file descriptor
  controllerFileDescriptor = i2cOpen(PCA9685_I2C_BUS_CHANNEL, PCA9685_I2C_ADDRESS, 0);
  if (controllerFileDescriptor < 0) {
    perror("Init failed.\n");
  }
  printf("Init success\n");

  reset();

  if (prescale != 0) {
    setExtClk(prescale);
  } else {
    // set a default frequency
    setPWMFreq(50);
  }
  // set the default internal frequency
  setOscillatorFrequency(FREQUENCY_OSCILLATOR);
  return controllerFileDescriptor;
}

/*!
 * Released control on the i2c device. Call before program exit.
 * */
void servoDriverDeInit(unsigned int fd) {
  if (i2cClose(fd) != 0) {
    perror("Unable to close device, not open");
  }
}


/*!
 *  @brief  Sends a reset command to the PCA9685 chip over I2C
 */
void reset() {
  write8(PCA9685_MODE1, MODE1_RESTART);
  //sleep(10);
}

/*!
 *  @brief  Puts board into sleep mode
 */
void servoDriverSleep() {
  uint8_t awake = read8(PCA9685_MODE1);
  uint8_t sleep = awake | MODE1_SLEEP; // set sleep bit high
  write8(PCA9685_MODE1, sleep);
  //sleep(5); // wait until cycle ends for sleep to be active
}

/*!
 *  @brief  Wakes board from sleep
 */
void wakeup() {
  uint8_t sleep = read8(PCA9685_MODE1);
  uint8_t wakeup = sleep & ~MODE1_SLEEP; // set sleep bit low
  write8(PCA9685_MODE1, wakeup);
}

/*!
 *  @brief  Sets EXTCLK pin to use the external clock
 *  @param  prescale
 *          Configures the prescale value to be used by the external clock
 */
void setExtClk(uint8_t prescale) {
  uint8_t oldmode = read8(PCA9685_MODE1);
  uint8_t newmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP; // sleep
  write8(PCA9685_MODE1, newmode);                             // go to sleep, turn off internal oscillator

  // This sets both the SLEEP and EXTCLK bits of the MODE1 register to switch to
  // use the external clock.
  write8(PCA9685_MODE1, (newmode |= MODE1_EXTCLK));

  write8(PCA9685_PRESCALE, prescale); // set the prescaler

  //sleep(5);
  // clear the SLEEP bit to start
  write8(PCA9685_MODE1, (newmode & ~MODE1_SLEEP) | MODE1_RESTART | MODE1_AI);
}

/*!
 *  @brief  Sets the PWM frequency for the entire chip, up to ~1.6 KHz
 *  @param  freq Floating point frequency that we will attempt to match
 */
void setPWMFreq(float freq) {
  // Range output modulation frequency is dependant on oscillator
  if (freq < 1)
    freq = 1;
  if (freq > 3500)
    freq = 3500; // Datasheet limit is 3052=50MHz/(4*4096)

  float prescaleval = ((_oscillator_freq / (freq * 4096.0)) + 0.5) - 1;
  if (prescaleval < PCA9685_PRESCALE_MIN)
    prescaleval = PCA9685_PRESCALE_MIN;
  if (prescaleval > PCA9685_PRESCALE_MAX)
    prescaleval = PCA9685_PRESCALE_MAX;
  uint8_t prescale = (uint8_t)prescaleval;

  uint8_t oldmode = read8(PCA9685_MODE1);
  uint8_t newmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP; // sleep
  write8(PCA9685_MODE1, newmode);                             // go to sleep
  write8(PCA9685_PRESCALE, prescale);                         // set the prescaler
  write8(PCA9685_MODE1, oldmode);
  //sleep(5);
  // This sets the MODE1 register to turn on auto increment.
  write8(PCA9685_MODE1, oldmode | MODE1_RESTART | MODE1_AI);
}

/*!
 *  @brief  Sets the output mode of the PCA9685 to either
 *  open drain or push pull / totempole.
 *  Warning: LEDs with integrated zener diodes should
 *  only be driven in open drain mode.
 *  @param  totempole Totempole if true, open drain if false.
 */
void setOutputMode(uint8_t totempole) {
  uint8_t oldmode = read8(PCA9685_MODE2);
  uint8_t newmode;
  if (totempole) {
    newmode = oldmode | MODE2_OUTDRV;
  } else {
    newmode = oldmode & ~MODE2_OUTDRV;
  }
  write8(PCA9685_MODE2, newmode);
}

/*!
 *  @brief  Reads set Prescale from PCA9685
 *  @return prescale value
 */
uint8_t readPrescale(void) {
  return read8(PCA9685_PRESCALE);
}

/*!
 *  @brief  Gets the PWM output of one of the PCA9685 pins
 *  @param  num One of the PWM output pins, from 0 to 15
 *  @return requested PWM output value
 */
uint8_t getPWM(uint8_t num) {
  return read8(PCA9685_LED0_ON_L + 4 * num);
}

/*!
 *  @brief  Sets the PWM output of one of the PCA9685 pins
 *  @param  num One of the PWM output pins, from 0 to 15
 *  @param  on At what point in the 4096-part cycle to turn the PWM output ON
 *  @param  off At what point in the 4096-part cycle to turn the PWM output OFF
 */
void setPWM(uint8_t num, uint16_t on, uint16_t off) {
  // Leave this as uint8_t even if compiler warns
  char toWrite[5] = {
      PCA9685_LED0_ON_L + 4 * num,
      on,
      on >> 8,
      off,
      off >> 8};
  // write(controllerFileDescriptor, toWrite, 5);
  i2cWriteDevice(controllerFileDescriptor, toWrite, 5);
}

void servoDriverWriteCommands() {
  int bytesToWrite = (SERVO_COUNT * 4) + 1;
  char toWrite[bytesToWrite];
  toWrite[0] = PCA9685_LED0_ON_L;
  
  for (int i = 0; i < SERVO_COUNT; i++) {
    uint16_t servoPwm = (((SG90_MAX - SG90_MIN) / (180 - 0)) * (unsigned int)servoPositions[i]) + SG90_MIN;
    //cout << (unsigned int)servoPositions[i] << '\t';
    for (int j = 0; j < 4; j++) {
      int index = j * 4 + 1;
      toWrite[index] = 0;
      toWrite[index + 1] = 0 >> 8;
      toWrite[index + 2] = servoPwm;
      toWrite[index + 3] = servoPwm >> 8;
    }
    cout << '\r' ;
  }
  

  // Left for example
  // uint16_t wheel4Pwm = (controlCommand.motorY2 * 1.0 / 255) * PCA9685_MAX_PWM;
  // uint16_t wheel4D1 = (controlCommand.motorY2D1 == 1) ? PCA9685_MAX_PWM : 0;
  // uint16_t wheel4D2 = (controlCommand.motorY2D2 == 1) ? PCA9685_MAX_PWM : 0;
  
  
  i2cWriteDevice(controllerFileDescriptor, toWrite, bytesToWrite);
}

/*!
 * Gives a more intuitive interface to setPwm function.
 * Valid only for 180 degree commonly used servos.
 * Tested with the SG90 small 9g servo.
 * Accepts values 0 - 180 (inclusive)
 */
void setPos(uint8_t servoNum, uint8_t position) {
  if (position > (uint8_t)180)
    position = (uint8_t)180;

  uint16_t correspondingPwm = (((SG90_MAX - SG90_MIN) / (180 - 0)) * position) + SG90_MIN;
  setPWM(servoNum, 0, correspondingPwm);
}

/*!
 *   @brief  Helper to set pin PWM output. Sets pin without having to deal with
 * on/off tick placement and properly handles a zero value as completely off and
 * 4095 as completely on.  Optional invert parameter supports inverting the
 * pulse for sinking to ground.
 *   @param  num One of the PWM output pins, from 0 to 15
 *   @param  val The number of ticks out of 4096 to be active, should be a value
 * from 0 to 4095 inclusive.
 *   @param  invert If true, inverts the output, defaults to 'false'
 */
void setPin(uint8_t num, uint16_t val, uint8_t invert) {
  invert = 0;
  // Clamp value between 0 and 4095 inclusive.
  val = (val < (uint16_t)4095) ? val : (uint16_t)4095;
  if (invert) {
    if (val == 0) {
      // Special value for signal fully on.
      setPWM(num, 4096, 0);
    } else if (val == 4095) {
      // Special value for signal fully off.
      setPWM(num, 0, 4096);
    } else {
      setPWM(num, 0, 4095 - val);
    }
  } else {
    if (val == 4095) {
      // Special value for signal fully on.
      setPWM(num, 4096, 0);
    } else if (val == 0) {
      // Special value for signal fully off.
      setPWM(num, 0, 4096);
    } else {
      setPWM(num, 0, val);
    }
  }
}

/*!
 *  @brief  Getter for the internally tracked oscillator used for freq
 * calculations
 *  @returns The frequency the PCA9685 thinks it is running at (it cannot
 * introspect)
 */
uint32_t getOscillatorFrequency(void) {
  return _oscillator_freq;
}

/*!
 *  @brief Setter for the internally tracked oscillator used for freq
 * calculations
 *  @param freq The frequency the PCA9685 should use for frequency calculations
 */
void setOscillatorFrequency(uint32_t freq) {
  _oscillator_freq = freq;
}

/******************* Low level I2C interface */
uint8_t read8(uint8_t addr) {
  return i2cReadByteData(controllerFileDescriptor, addr);
}

void write8(uint8_t addr, uint8_t value) {
  if (i2cWriteByteData(controllerFileDescriptor, addr, value) < 0) {
    perror("Unable to write byte");
  }
}


#else

// Test code. Replaces most i2c code with some print statements

int controllerFileDescriptor = -1;
uint32_t _oscillator_freq = FREQUENCY_OSCILLATOR;
//uint8_t *servoPositions;

/*!
 *  @brief  Setups the I2C interface and hardware
 *  @param  prescale
 *          Sets External Clock (Optional)
 */
int servoDriverInit(uint8_t prescale) {
  // Init servo position array
  servoPositions = new char[SERVO_COUNT];
  for (int i = 0; i < SERVO_COUNT; i++) {
	  servoPositions[i] = 1;
  }
  
  printf("Init success\n");

  reset();

  if (prescale != 0) {
    setExtClk(prescale);
  } else {
    // set a default frequency
    setPWMFreq(50);
  }
  // set the default internal frequency
  setOscillatorFrequency(FREQUENCY_OSCILLATOR);
  return controllerFileDescriptor;
}

/*!
 * Released control on the i2c device. Call before program exit.
 * */
void servoDriverDeInit(unsigned int fd) {
  cout << "Deinit servo driver\n";
}


/*!
 *  @brief  Sends a reset command to the PCA9685 chip over I2C
 */
void reset() {
  write8(PCA9685_MODE1, MODE1_RESTART);
  //sleep(10);
}

/*!
 *  @brief  Puts board into sleep mode
 */
void servoDriverSleep() {
  uint8_t awake = read8(PCA9685_MODE1);
  uint8_t sleep = awake | MODE1_SLEEP; // set sleep bit high
  write8(PCA9685_MODE1, sleep);
  //sleep(5); // wait until cycle ends for sleep to be active
}

/*!
 *  @brief  Wakes board from sleep
 */
void wakeup() {
  uint8_t sleep = read8(PCA9685_MODE1);
  uint8_t wakeup = sleep & ~MODE1_SLEEP; // set sleep bit low
  write8(PCA9685_MODE1, wakeup);
}

/*!
 *  @brief  Sets EXTCLK pin to use the external clock
 *  @param  prescale
 *          Configures the prescale value to be used by the external clock
 */
void setExtClk(uint8_t prescale) {
  uint8_t oldmode = read8(PCA9685_MODE1);
  uint8_t newmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP; // sleep
  write8(PCA9685_MODE1, newmode);                             // go to sleep, turn off internal oscillator

  // This sets both the SLEEP and EXTCLK bits of the MODE1 register to switch to
  // use the external clock.
  write8(PCA9685_MODE1, (newmode |= MODE1_EXTCLK));

  write8(PCA9685_PRESCALE, prescale); // set the prescaler

  //sleep(5);
  // clear the SLEEP bit to start
  write8(PCA9685_MODE1, (newmode & ~MODE1_SLEEP) | MODE1_RESTART | MODE1_AI);
}

/*!
 *  @brief  Sets the PWM frequency for the entire chip, up to ~1.6 KHz
 *  @param  freq Floating point frequency that we will attempt to match
 */
void setPWMFreq(float freq) {
  // Range output modulation frequency is dependant on oscillator
  if (freq < 1)
    freq = 1;
  if (freq > 3500)
    freq = 3500; // Datasheet limit is 3052=50MHz/(4*4096)

  float prescaleval = ((_oscillator_freq / (freq * 4096.0)) + 0.5) - 1;
  if (prescaleval < PCA9685_PRESCALE_MIN)
    prescaleval = PCA9685_PRESCALE_MIN;
  if (prescaleval > PCA9685_PRESCALE_MAX)
    prescaleval = PCA9685_PRESCALE_MAX;
  uint8_t prescale = (uint8_t)prescaleval;

  uint8_t oldmode = read8(PCA9685_MODE1);
  uint8_t newmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP; // sleep
  write8(PCA9685_MODE1, newmode);                             // go to sleep
  write8(PCA9685_PRESCALE, prescale);                         // set the prescaler
  write8(PCA9685_MODE1, oldmode);
  //sleep(5);
  // This sets the MODE1 register to turn on auto increment.
  write8(PCA9685_MODE1, oldmode | MODE1_RESTART | MODE1_AI);
}

/*!
 *  @brief  Sets the output mode of the PCA9685 to either
 *  open drain or push pull / totempole.
 *  Warning: LEDs with integrated zener diodes should
 *  only be driven in open drain mode.
 *  @param  totempole Totempole if true, open drain if false.
 */
void setOutputMode(uint8_t totempole) {
  uint8_t oldmode = read8(PCA9685_MODE2);
  uint8_t newmode;
  if (totempole) {
    newmode = oldmode | MODE2_OUTDRV;
  } else {
    newmode = oldmode & ~MODE2_OUTDRV;
  }
  write8(PCA9685_MODE2, newmode);
}

/*!
 *  @brief  Reads set Prescale from PCA9685
 *  @return prescale value
 */
uint8_t readPrescale(void) {
  return read8(PCA9685_PRESCALE);
}

/*!
 *  @brief  Gets the PWM output of one of the PCA9685 pins
 *  @param  num One of the PWM output pins, from 0 to 15
 *  @return requested PWM output value
 */
uint8_t getPWM(uint8_t num) {
  return read8(PCA9685_LED0_ON_L + 4 * num);
}

/*!
 *  @brief  Sets the PWM output of one of the PCA9685 pins
 *  @param  num One of the PWM output pins, from 0 to 15
 *  @param  on At what point in the 4096-part cycle to turn the PWM output ON
 *  @param  off At what point in the 4096-part cycle to turn the PWM output OFF
 */
void setPWM(uint8_t num, uint16_t on, uint16_t off) {
  // Leave this as uint8_t even if compiler warns
  uint8_t toWrite[5] = {
      PCA9685_LED0_ON_L + 4 * num,
      on,
      on >> 8,
      off,
      off >> 8};
  
  cout << "Setting PWM pin:" << num << '\n';
}

void servoDriverWriteCommands() {
  int bytesToWrite = (SERVO_COUNT * 4) + 1;
  char toWrite[bytesToWrite];
  toWrite[0] = PCA9685_LED0_ON_L;
  
  for (int i = 0; i < SERVO_COUNT; i++) {
    uint16_t servoPwm = (((SG90_MAX - SG90_MIN) / (180 - 0)) * (unsigned int)servoPositions[i]) + SG90_MIN;
    cout << (unsigned int)servoPositions[i] << '\t';
    for (int j = 0; j < 4; j++) {
      int index = j * 4 + 1;
      toWrite[index] = 0;
      toWrite[index + 1] = 0 >> 8;
      toWrite[index + 2] = servoPwm;
      toWrite[index + 3] = servoPwm >> 8;
    }
  }
  cout << '\n' ;
  
}

/*!
 * Gives a more intuitive interface to setPwm function.
 * Valid only for 180 degree commonly used servos.
 * Tested with the SG90 small 9g servo.
 * Accepts values 0 - 180 (inclusive)
 */
void setPos(uint8_t servoNum, uint8_t position) {
  if (position > (uint8_t)180)
    position = (uint8_t)180;

  uint16_t correspondingPwm = (((SG90_MAX - SG90_MIN) / (180 - 0)) * position) + SG90_MIN;
  setPWM(servoNum, 0, correspondingPwm);
}

/*!
 *   @brief  Helper to set pin PWM output. Sets pin without having to deal with
 * on/off tick placement and properly handles a zero value as completely off and
 * 4095 as completely on.  Optional invert parameter supports inverting the
 * pulse for sinking to ground.
 *   @param  num One of the PWM output pins, from 0 to 15
 *   @param  val The number of ticks out of 4096 to be active, should be a value
 * from 0 to 4095 inclusive.
 *   @param  invert If true, inverts the output, defaults to 'false'
 */
void setPin(uint8_t num, uint16_t val, uint8_t invert) {
  invert = 0;
  // Clamp value between 0 and 4095 inclusive.
  val = (val < (uint16_t)4095) ? val : (uint16_t)4095;
  if (invert) {
    if (val == 0) {
      // Special value for signal fully on.
      setPWM(num, 4096, 0);
    } else if (val == 4095) {
      // Special value for signal fully off.
      setPWM(num, 0, 4096);
    } else {
      setPWM(num, 0, 4095 - val);
    }
  } else {
    if (val == 4095) {
      // Special value for signal fully on.
      setPWM(num, 4096, 0);
    } else if (val == 0) {
      // Special value for signal fully off.
      setPWM(num, 0, 4096);
    } else {
      setPWM(num, 0, val);
    }
  }
}

/*!
 *  @brief  Getter for the internally tracked oscillator used for freq
 * calculations
 *  @returns The frequency the PCA9685 thinks it is running at (it cannot
 * introspect)
 */
uint32_t getOscillatorFrequency(void) {
  return _oscillator_freq;
}

/*!
 *  @brief Setter for the internally tracked oscillator used for freq
 * calculations
 *  @param freq The frequency the PCA9685 should use for frequency calculations
 */
void setOscillatorFrequency(uint32_t freq) {
  _oscillator_freq = freq;
}

/******************* Low level I2C interface */
uint8_t read8(uint8_t addr) {
  return 1;
}

void write8(uint8_t addr, uint8_t value) {
  
}

#endif
