#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Teste ADS1115");

  if (!ads.begin()) {
    Serial.println("ADS1115 NAO encontrado!");
    while (1);
  }

  Serial.println("ADS1115 conectado com sucesso!");
}

void loop() {

  int16_t ch0 = ads.readADC_SingleEnded(0);
  int16_t ch1 = ads.readADC_SingleEnded(1);
  int16_t ch2 = ads.readADC_SingleEnded(2);
  int16_t ch3 = ads.readADC_SingleEnded(3);

  Serial.print("A0: ");
  Serial.print(ch0);

  Serial.print(" | A1: ");
  Serial.print(ch1);

  Serial.print(" | A2: ");
  Serial.print(ch2);

  Serial.print(" | A3: ");
  Serial.println(ch3);

  delay(1000);
}