/*!
 *  @file Adafruit_MPL3115A2.cpp
 *
 *  @mainpage Adafruit MPL3115A2 alitmeter
 *
 *  @section intro_sec Introduction
 *
 *  This is the documentation for Adafruit's MPL3115A2 driver for the
 *  Arduino platform.  It is designed specifically to work with the
 *  Adafruit MPL3115A2 breakout: https://www.adafruit.com/products/1893
 *
 *  These sensors use I2C to communicate, 2 pins (SCL+SDA) are required
 *  to interface with the breakout.
 *
 *  Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing
 *  products from Adafruit!
 *
 *  @section dependencies Dependencies
 *
 *  @section author Author
 *
 *  Written by Kevin "KTOWN" Townsend for Adafruit Industries.
 *
 *  @section license License
 *
 *  BSD license, all text here must be included in any redistribution.
 *
 */

#include "Adafruit_MPL3115A2.h"

/*!
    @brief  Instantiates a new MPL3115A2 class
*/
Adafruit_MPL3115A2::Adafruit_MPL3115A2() {}

/*!
 *   @brief  Setups the HW (reads coefficients values, etc.)
 *   @param  twoWire
 *           Optional TwoWire I2C object
 *   @return true on successful startup, false otherwise
 */
boolean Adafruit_MPL3115A2::begin(TwoWire *twoWire) {
  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(MPL3115A2_ADDRESS, twoWire);
  if (!i2c_dev->begin())
    return false;

  // sanity check
  uint8_t whoami = read8(MPL3115A2_WHOAMI);
  if (whoami != 0xC4) {
    return false;
  }

  // software reset
  write8(MPL3115A2_CTRL_REG1, MPL3115A2_CTRL_REG1_RST);
  while (read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_RST)
    delay(10);

  // set oversampling and altitude mode
  _ctrl_reg1.reg = MPL3115A2_CTRL_REG1_OS128 | MPL3115A2_CTRL_REG1_ALT;
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);

  // enable data ready events for pressure/altitude and temperature
  write8(MPL3115A2_PT_DATA_CFG, MPL3115A2_PT_DATA_CFG_TDEFE |
                                    MPL3115A2_PT_DATA_CFG_PDEFE |
                                    MPL3115A2_PT_DATA_CFG_DREM);

  return true;
}

/*!
 *  @brief  Get barometric pressure
 *  @return pressure reading as a floating point value in hPa
 */
float Adafruit_MPL3115A2::getPressure() {
  // wait for one-shot to clear before proceeding
  while (read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_OST)
    delay(10);

  // configure and initiate measurement
  _ctrl_reg1.bit.ALT = 0; // barometer (pressure) mode
  _ctrl_reg1.bit.OST = 1; // initatiate a one-shot measurement
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);

  // poll status to wait for conversion complete
  while (!(read8(MPL3115A2_REGISTER_STATUS) & MPL3115A2_REGISTER_STATUS_PDR))
    delay(10);

  // read data
  uint32_t pressure;
  uint8_t buffer[5] = {MPL3115A2_REGISTER_PRESSURE_MSB, 0, 0, 0, 0};
  i2c_dev->write_then_read(buffer, 1, buffer, 5);
  pressure = uint32_t(buffer[0]) << 16 | uint32_t(buffer[1]) << 8 |
             uint32_t(buffer[2]);
  return float(pressure) / 6400.0;
}

/*!
 *  @brief  Get altitude
 *  @return altitude reading as a floating-point value in meters
 */
float Adafruit_MPL3115A2::getAltitude() {
  // wait for one-shot to clear before proceeding
  while (read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_OST)
    delay(10);

  // configure and initiate measurement
  _ctrl_reg1.bit.ALT = 1; // altimeter mode
  _ctrl_reg1.bit.OST = 1; // initatiate a one-shot measurement
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);

  // poll status to wait for conversion complete
  while (!(read8(MPL3115A2_REGISTER_STATUS) & MPL3115A2_REGISTER_STATUS_PDR))
    delay(10);

  // read data
  int32_t alt;
  uint8_t buffer[5] = {MPL3115A2_REGISTER_PRESSURE_MSB, 0, 0, 0, 0};
  i2c_dev->write_then_read(buffer, 1, buffer, 5);
  alt = uint32_t(buffer[0]) << 24 | uint32_t(buffer[1]) << 16 |
        uint32_t(buffer[2]) << 8;
  return float(alt) / 65536.0;
}

/*!
 *  @brief  Set the local sea level pressure
 *  @param SLP sea level pressure in hPa
 */
void Adafruit_MPL3115A2::setSeaPressure(float SLP) {
  // multiply by 100 to convert hPa to Pa
  // divide by 2 to convert to 2 Pa per LSB
  // convert to integer
  uint16_t bar = SLP * 50;

  // write result to register
  uint8_t buffer[3];
  buffer[0] = MPL3115A2_BAR_IN_MSB;
  buffer[1] = bar >> 8;
  buffer[2] = bar & 0xFF;
  i2c_dev->write(buffer, 3);
}

/*!
 *  @brief  Get temperature
 *  @return temperature reading as a floating-point value in degC
 */
float Adafruit_MPL3115A2::getTemperature() {
  // wait for one-shot to clear before proceeding
  while (read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_OST)
    delay(10);

  // initatiate a one-shot measurement
  _ctrl_reg1.bit.OST = 1;
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);

  // poll status to wait for conversion complete
  while (!(read8(MPL3115A2_REGISTER_STATUS) & MPL3115A2_REGISTER_STATUS_PTDR))
    delay(10);

  // read data
  int16_t t;
  uint8_t buffer[5] = {MPL3115A2_REGISTER_PRESSURE_MSB, 0, 0, 0, 0};
  i2c_dev->write_then_read(buffer, 1, buffer, 5);
  t = uint16_t(buffer[3]) << 8 | uint16_t(buffer[4]);
  return float(t) / 256.0;
}

/*!
 *  @brief  read 1 byte of data at the specified address
 *  @param  a
 *          the address to read
 *  @return the read data byte
 */
uint8_t Adafruit_MPL3115A2::read8(uint8_t a) {
  uint8_t buffer[1] = {a};
  i2c_dev->write_then_read(buffer, 1, buffer, 1);
  return buffer[0];
}

/*!
 *  @brief  write a byte of data to the specified address
 *  @param  a
 *          the address to write to
 *  @param  d
 *          the byte to write
 */
void Adafruit_MPL3115A2::write8(uint8_t a, uint8_t d) {
  uint8_t buffer[2] = {a, d};
  i2c_dev->write(buffer, 2);
}
