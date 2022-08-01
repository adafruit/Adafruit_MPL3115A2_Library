/**
 * Async example for MPL3115A2
 */

#include <Adafruit_MPL3115A2.h>

Adafruit_MPL3115A2 mpl;

void setup() {
  Serial.begin(9600);
  while(!Serial);
  Serial.println("Adafruit_MPL3115A2 test!");

  if (!mpl.begin()) {
    Serial.println("Could not find sensor. Check wiring.");
    while(1);
  }

  // set mode before starting a conversion
  Serial.println("Setting mode to barometer (pressure).");
  mpl.setMode(MPL3115A2_BAROMETER);
}

void loop() {
  // start a conversion
  Serial.println("Starting a conversion.");
  mpl.startOneShot();

  // do something else while waiting
  Serial.println("Counting number while waiting...");
  int count = 0;
  while (!mpl.conversionComplete()) {
    count++;
  }
  Serial.print("Done! Counted to "); Serial.println(count);

  // now get results
  Serial.print("Pressure = ");
  Serial.println(mpl.getLastConversionResults(MPL3115A2_PRESSURE));
  Serial.print("Temperature = ");
  Serial.println(mpl.getLastConversionResults(MPL3115A2_TEMPERATURE));
  Serial.println();

  delay(1000);
}