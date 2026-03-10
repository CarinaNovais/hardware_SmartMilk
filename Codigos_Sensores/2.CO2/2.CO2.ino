#include <Wire.h>
#include "Adafruit_CCS811.h"

Adafruit_CCS811 ccs;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Iniciando CCS811...");

  Wire.begin(21, 22);  // SDA = 21, SCL = 22

  if (!ccs.begin()) {
    Serial.println("Erro ao iniciar o sensor CCS811! Verifique conexões.");
    while (1);
  }

  Serial.println("Sensor encontrado! Aguardando estabilizar...");
  
  // Aguardar até o sensor estar pronto
  while (!ccs.available());
}

void loop() {
  if (ccs.available()) {
    if (!ccs.readData()) {
      Serial.print("CO2: ");
      Serial.print(ccs.geteCO2());
      Serial.print(" ppm   TVOC: ");
      Serial.print(ccs.getTVOC());
      Serial.println(" ppb");
    } else {
      Serial.println("Erro ao ler dados do CCS811!");
    }
  }

  delay(1000);
}
