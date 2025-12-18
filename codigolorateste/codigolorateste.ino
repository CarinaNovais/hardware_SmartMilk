#include <SPI.h>
#include <LoRa.h>

// Pinos LoRa
#define LORA_SCK  18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS   5
#define LORA_RST  26
#define LORA_DIO0 2

// Frequência
#define LORA_FREQ 868E6

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Inicializando LoRa...");

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("Erro ao iniciar LoRa!");
    while (true); // Para aqui se não inicializar
  }

  Serial.println("LoRa inicializado com sucesso!");
}

int counter = 0;

void loop() {
  // Envia mensagem
  String mensagem = "Ola LoRa! Contador: " + String(counter);
  LoRa.beginPacket();
  LoRa.print(mensagem);
  LoRa.endPacket();

  Serial.println("Mensagem enviada: " + mensagem);
  counter++;

  // Recebe mensagem
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("Mensagem recebida: ");
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    Serial.println();
  }

  delay(2000); // Envia a cada 2 segundos
}
