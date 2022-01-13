/**************************************************************************/
/*!
    @file     Adafruit_MPL3115A2.cpp
    @author   K.Townsend (Adafruit Industries)
    @license  BSD (see license.txt)

    Example for the MPL3115A2 barometric pressure sensor

    This is a library for the Adafruit MPL3115A2 breakout
    ----> https://www.adafruit.com/products/1893

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!

    @section  HISTORY

    v1.0 - First release
*/
/**************************************************************************/

#include <Adafruit_MPL3115A2.h>

#undef TESTNONBLOCKING

Adafruit_MPL3115A2 baro;

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;
  Serial.println("Adafruit_MPL3115A2 test!");

  if (!baro.begin()) {
    Serial.println("Could not find sensor. Check wiring.");
    while (1)
      ;
  }

  // use to set sea level pressure for current location
  // this is needed for accurate altitude measurement
  // STD SLP = 1013.26 hPa
  baro.setSeaPressure(1013.26);
}

#if defined(TESTNONBLOCKING)
void loop() {
  float pressure = 0;
  uint8_t phase = baro.getPressureNonBlocking(pressure);
  // If phase is zero, we have valid data, otherwise, we need to call again.
  if (!phase) {
    Serial.print(pressure);
    Serial.println(" hPa");
  } else
    Serial.println("...");
  delay(100);
}
#else
void loop() {
  float pressure = baro.getPressure();
  float altitude = baro.getAltitude();
  float temperature = baro.getTemperature();

  Serial.println("-----------------");
  Serial.print("pressure = ");
  Serial.print(pressure);
  Serial.println(" hPa");
  Serial.print("altitude = ");
  Serial.print(altitude);
  Serial.println(" m");
  Serial.print("temperature = ");
  Serial.print(temperature);
  Serial.println(" C");

  delay(250);
}
#endif
