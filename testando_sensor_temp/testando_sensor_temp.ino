#include <OneWire.h>

#define PINO_TEMP 25
OneWire oneWire(PINO_TEMP);

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nOneWire Scanner - buscando devices...");
}

void loop() {
  byte addr[8];
  int count = 0;
  oneWire.reset_search();
  while (oneWire.search(addr)) {
    Serial.print("Encontrado device #");
    Serial.print(count);
    Serial.print(" - endereco: ");
    for (int i = 0; i < 8; i++) {
      if (addr[i] < 16) Serial.print("0");
      Serial.print(addr[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    count++;
  }
  Serial.print("Total encontrados: ");
  Serial.println(count);
  if (count == 0) Serial.println(">> Nenhum DS18B20 detectado. Verifique VCC/GND/DATA/pull-up.");
  delay(3000);
}
