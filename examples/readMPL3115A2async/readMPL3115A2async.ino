/**************************************************************************/
/*!
    @file     Adafruit_MPL3115A2async.cpp
    @author   A. Fitzhugh
    @license  BSD (see license.txt)

    Example for the MPL3115A2 barometric pressure sensor, being read in polled mode

    This is a library for the Adafruit MPL3115A2 breakout
    ----> https://www.adafruit.com/products/1893

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!

    @section  HISTORY

    v1.0 - First release
*/
/**************************************************************************/

#include "Adafruit_MPL3115A2.h"

// Power by connecting Vin to 3-5V, GND to GND
// Uses I2C - connect SCL to the SCL pin, SDA to SDA pin
// See the Wire tutorial for pinouts for each Arduino
// http://arduino.cc/en/reference/wire
Adafruit_MPL3115A2async baro = Adafruit_MPL3115A2async();

// a counter to show how many loops we execute between sensor updates
unsigned long counter;
// and record the time
unsigned long timer;
unsigned long maxtime;
unsigned long mintime;

void setup() {
  Serial.begin(115200);
  Serial.println("Adafruit_MPL3115A2 test!");

  // we're need to initialise this here so we can set the clock speed later
  TwoWire myi2c;

  // we don't need to block here - baro.poll is quick if the initialisation wasn't succesfull
  if (!baro.begin(&myi2c)) {
    Serial.println("Couldnt find sensor");
  }
  
  // a lot of the time for the loop is determined by i2c communications - set this as high as possible.
  myi2c.setClock(400000);    // fast mode

  // initialise stats
  counter = 0;
  maxtime = 0;
  mintime = -1;
}

void loop() {

  // poll is required to update data in background.
  // Here we are using quick=true to make loop run as fast as possible
  unsigned long start = micros();
  baro.poll(true);
  // read the data. Doesn't wait and we'll automatically get updated data when available
  float pascals = baro.getPressure();
  float tempC = baro.getTemperature();
  float altm = baro.getAltitude();
  unsigned long duration = micros() - start;

  // update the counter
  counter++;


  // update max/min
  if (duration > maxtime) maxtime = duration; 
  if (duration < mintime) mintime = duration; 

  // isNewData() shows when we have new data available and can be used to trigger further actions
  if (baro.isNewData()){ 
    // display data
    Serial.print(pascals/1000); Serial.print(" Pressure (kPa)     ");
    Serial.print(altm); Serial.print(" meters     ");
    Serial.print(tempC); Serial.print("*C     ");
    Serial.print(counter); Serial.print(" loops executed of which, sensor update took ");
    Serial.print(mintime); Serial.print(" to ");
    Serial.print(maxtime); Serial.print(" microseconds");
    // and goto next line
    Serial.println();

    // needed only to reset isNewData()
    baro.reset();             

    // reset stats
    counter = 0;
    maxtime = 0;
    mintime = -1;
  }
}