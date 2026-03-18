#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 26
#define LORA_DIO0 33
#define LORA_FREQ 915E6

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Serial.println("Inicializando LoRa...");

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ))
  {
    Serial.println("Erro LoRa");
    while (1);
  }

  Serial.println("LoRa OK");
}

void loop()
{
  LoRa.beginPacket();
  LoRa.print("teste lora");
  LoRa.endPacket();

  Serial.println("Pacote enviado");

  delay(2000);
}