/*!
 * @file Adafruit_MPL3115A2.cpp
 *
 * @mainpage Adafruit MPL3115A2 alitmeter
 *
 * @section intro_sec Introduction
 *
 * This is the documentation for Adafruit's MPL3115A2 driver for the
 * Arduino platform.  It is designed specifically to work with the
 * Adafruit MPL3115A2 breakout: https://www.adafruit.com/products/1893
 *
 * These sensors use I2C to communicate, 2 pins (SCL+SDA) are required
 * to interface with the breakout.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section dependencies Dependencies
 *
 * @section author Author
 *
 * Written by Kevin "KTOWN" Townsend for Adafruit Industries.
 * Updated with code for polled mode by A. Fitzhugh.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Wire.h>

#include "Adafruit_MPL3115A2.h"

/**************************************************************************/
/*!
    @brief  Instantiates a new MPL3115A2 class
*/
/**************************************************************************/
Adafruit_MPL3115A2::Adafruit_MPL3115A2() {

}

/**************************************************************************/
/*!
    @brief  Setups the HW (reads coefficients values, etc.)
    @param twoWire Optional TwoWire I2C object
    @return true on successful startup, false otherwise
*/
/**************************************************************************/
boolean Adafruit_MPL3115A2::begin(TwoWire *twoWire) {
  _i2c = twoWire;
  _i2c->begin();
  uint8_t whoami = read8(MPL3115A2_WHOAMI);
  if (whoami != 0xC4) {
    return false;
  }

  write8(MPL3115A2_CTRL_REG1, MPL3115A2_CTRL_REG1_RST);
  delay(10);

  while(read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_RST) delay(10);

  _ctrl_reg1.reg = MPL3115A2_CTRL_REG1_OS128 | MPL3115A2_CTRL_REG1_ALT;

  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);

  write8(MPL3115A2_PT_DATA_CFG, 
   MPL3115A2_PT_DATA_CFG_TDEFE |
   MPL3115A2_PT_DATA_CFG_PDEFE |
   MPL3115A2_PT_DATA_CFG_DREM);
  return true;
}

/**************************************************************************/
/*!
    @brief  Gets the floating-point pressure level in kPa
    @return altitude reading as a floating point value
*/
/**************************************************************************/
float Adafruit_MPL3115A2::getPressure() {
  uint32_t pressure;

  while(read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_OST) delay(10);

  _ctrl_reg1.bit.ALT = 0;
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);

  _ctrl_reg1.bit.OST = 1;
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);

  uint8_t sta = 0;
  while (! (sta & MPL3115A2_REGISTER_STATUS_PDR)) {
    sta = read8(MPL3115A2_REGISTER_STATUS);
    delay(10);
  }
  _i2c->beginTransmission(MPL3115A2_ADDRESS); // start transmission to device 
  _i2c->write(MPL3115A2_REGISTER_PRESSURE_MSB); 
  _i2c->endTransmission(false); // end transmission
  
  _i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)3);// send data n-bytes read
  pressure = _i2c->read(); // receive DATA
  pressure <<= 8;
  pressure |= _i2c->read(); // receive DATA
  pressure <<= 8;
  pressure |= _i2c->read(); // receive DATA
  pressure >>= 4;

  float baro = pressure;
  baro /= 4.0;
  return baro;
}

/**************************************************************************/
/*!
    @brief  Gets the floating-point altitude value
    @return altitude reading as a floating-point value
*/
/**************************************************************************/
float Adafruit_MPL3115A2::getAltitude() {
  int32_t alt;

  while(read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_OST) delay(10);

  _ctrl_reg1.bit.ALT = 1;
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);

  _ctrl_reg1.bit.OST = 1;
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);

  uint8_t sta = 0;
  while (! (sta & MPL3115A2_REGISTER_STATUS_PDR)) {
    sta = read8(MPL3115A2_REGISTER_STATUS);
    delay(10);
  }
  _i2c->beginTransmission(MPL3115A2_ADDRESS); // start transmission to device 
  _i2c->write(MPL3115A2_REGISTER_PRESSURE_MSB); 
  _i2c->endTransmission(false); // end transmission
  

  _i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)3);// send data n-bytes read
  alt  = ((uint32_t)_i2c->read()) << 24; // receive DATA
  alt |= ((uint32_t)_i2c->read()) << 16; // receive DATA
  alt |= ((uint32_t)_i2c->read()) << 8; // receive DATA

  float altitude = alt;
  altitude /= 65536.0;
  return altitude;
}

/**************************************************************************/
/*!
    @brief  Set the local sea level barometric pressure
    @param pascal the pressure to use as the baseline
*/
/**************************************************************************/
void Adafruit_MPL3115A2::setSeaPressure(float pascal) {
  uint16_t bar = pascal/2;
  _i2c->beginTransmission(MPL3115A2_ADDRESS);
  _i2c->write((uint8_t)MPL3115A2_BAR_IN_MSB);
  _i2c->write((uint8_t)(bar>>8));
  _i2c->write((uint8_t)bar);
  _i2c->endTransmission(false);
}

/**************************************************************************/
/*!
    @brief  Gets the floating-point temperature in Centigrade
    @return temperature reading in Centigrade as a floating-point value
*/
/**************************************************************************/
float Adafruit_MPL3115A2::getTemperature() {
  int16_t t;

  uint8_t sta = 0;
  while (! (sta & MPL3115A2_REGISTER_STATUS_TDR)) {
    sta = read8(MPL3115A2_REGISTER_STATUS);
    delay(10);
  }
  _i2c->beginTransmission(MPL3115A2_ADDRESS); // start transmission to device 
  _i2c->write(MPL3115A2_REGISTER_TEMP_MSB); 
  _i2c->endTransmission(false); // end transmission
  
  _i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)2);// send data n-bytes read
  t = _i2c->read(); // receive DATA
  t <<= 8;
  t |= _i2c->read(); // receive DATA
  t >>= 4;
  
  if (t & 0x800) {
    t |= 0xF000;
  }

  float temp = t;
  temp /= 16.0;
  return temp;
}

/**************************************************************************/
/*!
    @brief  read 1 byte of data at the specified address
    @param a the address to read
    @return the read data byte
*/
/**************************************************************************/
uint8_t Adafruit_MPL3115A2::read8(uint8_t a) {
  _i2c->beginTransmission(MPL3115A2_ADDRESS); // start transmission to device 
  _i2c->write(a); // sends register address to read from
  _i2c->endTransmission(false); // end transmission
  
  _i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)1);// send data n-bytes read
  return _i2c->read(); // receive DATA
}

/**************************************************************************/
/*!
    @brief  write a byte of data to the specified address
    @param a the address to write to
    @param d the byte to write
*/
/**************************************************************************/
void Adafruit_MPL3115A2::write8(uint8_t a, uint8_t d) {
  _i2c->beginTransmission(MPL3115A2_ADDRESS); // start transmission to device 
  _i2c->write(a); // sends register address to write to
  _i2c->write(d); // sends register data
  _i2c->endTransmission(false); // end transmission
}


/**************************************************************************/
/*!
    @brief  Instantiates a new MPL3115A2 class for polled mode
*/
/**************************************************************************/
Adafruit_MPL3115A2async::Adafruit_MPL3115A2async() {
}

/**************************************************************************/
/*!
    @brief returns the state of the polling cycle
    @return unsigned int representing the state of the reader (0 to 10 or 101-110) 
	    where 0 = uninitialised and >100 implies data has been refresehd since reset
*/
/**************************************************************************/
unsigned int Adafruit_MPL3115A2async::getstate(){
  return state;
}

/**************************************************************************/
/*!
    @brief  Setups the HW (reads coefficients values, etc.)
    @param twoWire Optional TwoWire I2C object
    @return true on successful startup, false otherwise
*/
/**************************************************************************/
boolean Adafruit_MPL3115A2async::begin(TwoWire *twoWire) {
  if (Adafruit_MPL3115A2::begin(twoWire)) {
    // record in the state that we have been initalised.
    state = 1;
    return true;
  } else {
    return false;
  }
}

/**************************************************************************/
/*!
    @brief  Gets the floating-point pressure level in kPa
    @return altitude reading as a floating point value
*/
/**************************************************************************/
float Adafruit_MPL3115A2async::getPressure() {
  return baro;
}
/**************************************************************************/
/*!
    @brief  Gets the floating-point altitude value
    @return altitude reading as a floating-point value
*/
/**************************************************************************/
float Adafruit_MPL3115A2async::getTemperature() {
  return temp;
}
/**************************************************************************/
/*!
    @brief  Gets the floating-point temperature in Centigrade
    @return temperature reading in Centigrade as a floating-point value
*/
/**************************************************************************/
float Adafruit_MPL3115A2async::getAltitude() {
  return altitude;
}
/**************************************************************************/
/*!
    @brief  Boolean indicating whether data has been refreshed since last reset
    @return true if data is new
*/
/**************************************************************************/
bool Adafruit_MPL3115A2async::isNewData() {
  return (state>=100);
}
/**************************************************************************/
/*!
    @brief  sets the data as old so we can tell when new data is read
    @param  reset pass true to reset, false to leave unchanged
*/
/**************************************************************************/
void Adafruit_MPL3115A2async::reset(bool reset) {
  if (reset && state>=100) state-=100;
}

/**************************************************************************/
/*!
    @brief  needs to be called regularly to update the pressures and temperatures
    @param  quick false is slower per call, fewer calls to refresh; true is faster per call, more calls to refresh
*/
/**************************************************************************/
void Adafruit_MPL3115A2async::poll(bool quick) {
  uint32_t pressure;
  int32_t alt;
  int16_t t;
  uint8_t sta;

  switch (state){
  default:     // unknown state - assume we aren't initialised
    state=0;
    break;
  case 0:     // we weren't initialised with begin(); do nothing.
    break;
  case 1:     // reading pressure   
  case 101:     // reading pressure   
    if ((read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_OST)) break;
    state++;
    if (quick) break;
  case 2:
  case 102:
    _ctrl_reg1.bit.ALT = 0;
    write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);
  
    _ctrl_reg1.bit.OST = 1;
    write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);
  
    state++;
    if (quick) break;
  case 3:     // 
  case 103:     // 
    sta = read8(MPL3115A2_REGISTER_STATUS);
    if (!(sta & MPL3115A2_REGISTER_STATUS_PDR)) break;
    state++;
    if (quick) break;
  case 4:
  case 104:
    _i2c->beginTransmission(MPL3115A2_ADDRESS); // start transmission to device 
    _i2c->write(MPL3115A2_REGISTER_PRESSURE_MSB); 
    _i2c->endTransmission(false); // end transmission
    
    _i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)3);// send data n-bytes read
    pressure = _i2c->read(); // receive DATA
    pressure <<= 8;
    pressure |= _i2c->read(); // receive DATA
    pressure <<= 8;
    pressure |= _i2c->read(); // receive DATA
    pressure >>= 4;
  
    baro = pressure;
    baro /= 4.0;
    state++;
    if (quick) break;
  case 5:     // read altitude 
  case 105:     // read altitude 
    if ((read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_OST)) break;
    state++;
    if (quick) break;
  case 6:
  case 106:
    _ctrl_reg1.bit.ALT = 1;
    write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);
  
    _ctrl_reg1.bit.OST = 1;
    write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);
  
    state++;
    if (quick) break;
  case 7:
  case 107:
    sta = read8(MPL3115A2_REGISTER_STATUS);
    if (! (sta & MPL3115A2_REGISTER_STATUS_PDR)) break;
    state++;
    if (!quick) break;
  case 8:
  case 108:
    _i2c->beginTransmission(MPL3115A2_ADDRESS); // start transmission to device 
    _i2c->write(MPL3115A2_REGISTER_PRESSURE_MSB); 
    _i2c->endTransmission(false); // end transmission
    
    _i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)3);// send data n-bytes read
    alt  = ((uint32_t)_i2c->read()) << 24; // receive DATA
    alt |= ((uint32_t)_i2c->read()) << 16; // receive DATA
    alt |= ((uint32_t)_i2c->read()) << 8; // receive DATA
  
    altitude = alt;
    altitude /= 65536.0;

    state++;
    if (quick) break;
  case 9:     // read temperature
  case 109:     // read temperature
    sta = read8(MPL3115A2_REGISTER_STATUS);
    if (! (sta & MPL3115A2_REGISTER_STATUS_TDR)) break;
    state++;
    if (quick) break;
  case 10:
  case 110:
    _i2c->beginTransmission(MPL3115A2_ADDRESS); // start transmission to device 
    _i2c->write(MPL3115A2_REGISTER_TEMP_MSB); 
    _i2c->endTransmission(false); // end transmission
    
    _i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)2);// send data n-bytes read
    t = _i2c->read(); // receive DATA
    t <<= 8;
    t |= _i2c->read(); // receive DATA
    t >>= 4;
    
    if (t & 0x800) {
      t |= 0xF000;
    }
  
    temp = t;
    temp /= 16.0;
    state=101;              // state >100 implies new data
    break;
  }
}
