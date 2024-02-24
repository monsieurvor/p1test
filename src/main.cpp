#include "settings.h"
#include "read_p1.h"

unsigned long lastReading = 0;

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial)
  {
    vTaskDelay(2);
  }
  Serial.println("Starting...");
  Serial2.begin(BAUD_RATE, SERIAL_8N1, RXD2, TXD2, true);
  while (!Serial2)
  {
    vTaskDelay(2);
  }
  Serial.println("Serial 2 started");
  setupDataReadout();
}

void loop()
{
    unsigned long now = millis();
    if (now - lastReading > READ_INTERVAL)
    {
        readP1Serial();
        lastReading = millis();
    }

}