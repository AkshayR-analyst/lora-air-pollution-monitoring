#ifndef ESP32
#include <SoftwareSerial.h>
#endif
#include "PMS7003-SOLDERED.h"

#define MQ135_PIN A0
#define MQ7_PIN A1
#define ard_node Serial // Hardware serial for ESP32

// define pins of the PMS7003
int PMS_RX = 2;
int PMS_TX = 3;

PMS7003 pms(PMS_RX, PMS_TX); // PMSx003, RX, TX
unsigned int pm1 = 0;
unsigned int pm2_5 = 0;
unsigned int pm10 = 0;

void setup()
{
  Serial.begin(115200);
  ard_node.begin(115200); // Hardware ard_node for ESP32

  Serial.println(F("PMS sensor on SWSerial"));
  Serial.print(F("  RX:"));
  Serial.println(PMS_RX);
  Serial.print(F("  TX:"));
  Serial.println(PMS_TX);

  pms.begin(); // Initialize serial communication with sensor
}

void loop()
{
  // Temperature, humidity and pressure sensors removed. No local vars for them.
  // int airQuality = analogRead(MQ135_PIN);

  int sensorValue = analogRead(MQ135_PIN);
  // MQ-7 sensor reading (not used in calculations)
  int mq7Value = analogRead(MQ7_PIN);

  int airQuality = sensorValue; // Use the raw analog value for air quality

  int Gas = mq7Value; // Use the raw analog value for CO detection

  // No DHT/BMP sensors configured; skip their error handling.

  // Serial.print("Air Quality: ");
  // Serial.print(airQuality);
  // Serial.println("ppm");
  // Serial.print("CO Level: ");
  // Serial.print(Gas);
  // Serial.println("ppm");
  // // Temperature/humidity/pressure not available
  // Serial.print("PM1: ");
  // Serial.print(pm1);
  // Serial.println("ug/m3");
  // Serial.print("PM2.5: ");
  // Serial.print(pm2_5);
  // Serial.println("ug/m3");
  // Serial.print("PM10: ");
  // Serial.print(pm10);
  // Serial.println("ug/m3");

  pms.read(); // read the PM sensor
  if (pms)
  { // successfull read
    // Serial.print(F("PM1.0 "));
    // Serial.print(pms.pm01); //Read PM0.1 particles concentration
    // Serial.print(F(", "));
    // Serial.print(F("PM2.5 "));
    // Serial.print(pms.pm25); //Read PM2.5 particles concentration
    // Serial.print(F(", "));
    // Serial.print(F("PM10 "));
    // Serial.print(pms.pm10); //Read PM10.0 particles concentration
    // Serial.println(F(" [ug/m3]"));

    if (pms.has_number_concentration())
    {
      // Serial.print(F("N0.3 "));
      // Serial.print(pms.n0p3);
      // Serial.print(F(", "));
      // Serial.print(F("N0.5 "));
      // Serial.print(pms.n0p5);
      // Serial.print(F(", "));
      // Serial.print(F("N1.0 "));
      // Serial.print(pms.n1p0);
      // Serial.print(F(", "));
      // Serial.print(F("N2.5 "));
      // Serial.print(pms.n2p5);
      // Serial.print(F(", "));
      // Serial.print(F("N5.0 "));
      // Serial.print(pms.n5p0);
      // Serial.print(F(", "));
      // Serial.print(F("N10 "));
      // Serial.print(pms.n10p0);
      // Serial.println(F(" [#/100cc]"));
    }

    if (pms.has_temperature_humidity() || pms.has_formaldehyde())
    {
      // Serial.print(pms.temp, 1);
      // Serial.print(F(" °C"));
      // Serial.print(F(", "));
      // Serial.print(pms.rhum, 1);
      // Serial.print(F(" %rh"));
      // Serial.print(F(", "));
      // Serial.print(pms.hcho, 2);
      // Serial.println(F(" mg/m3 HCHO"));
    }

    // update pm variables for CSV
    pm1 = pms.pm01;
    pm2_5 = pms.pm25;
    pm10 = pms.pm10;
  }
  // else
  // { // something went wrong
  //   switch (pms.status)
  //   {
  //   case pms.OK: // should never come here
  //     break;   // included to compile without warnings
  //   case pms.ERROR_TIMEOUT:
  //     Serial.println(F(PMS_ERROR_TIMEOUT));
  //     break;
  //   case pms.ERROR_MSG_UNKNOWN:
  //     Serial.println(F(PMS_ERROR_MSG_UNKNOWN));
  //     break;
  //   case pms.ERROR_MSG_HEADER:
  //     Serial.println(F(PMS_ERROR_MSG_HEADER));
  //     break;
  //   case pms.ERROR_MSG_BODY:
  //     Serial.println(F(PMS_ERROR_MSG_BODY));
  //     break;
  //   case pms.ERROR_MSG_START:
  //     Serial.println(F(PMS_ERROR_MSG_START));
  //     break;
  //   case pms.ERROR_MSG_LENGTH:
  //     Serial.println(F(PMS_ERROR_MSG_LENGTH));
  //     break;
  //   case pms.ERROR_MSG_CKSUM:
  //     Serial.println(F(PMS_ERROR_MSG_CKSUM));
  //     break;
  //   case pms.ERROR_PMS_TYPE:
  //     Serial.println(F(PMS_ERROR_PMS_TYPE));
  //     break;
  //   }
  // }
  // Create CSV data packet (temp/humidity/pressure removed)
  String data = String(airQuality) + "," + String(Gas) + "," + String(pm1) + "," + String(pm2_5) + "," + String(pm10) + ",";

  ard_node.println(data); // Send to ESP32
  delay(500);
  // Serial.println(" }");
}
