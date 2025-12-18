// =====================================================
// ESP1 - UART MASTER + LoRa TX (valores apenas)
// =====================================================

#include <Wire.h>
#include "Adafruit_CCS811.h"
#include <SPI.h>
#include <LoRa.h>

// ================= CONTROLE DE TEMPO =================
unsigned long ultimoEnvio = 0;
const unsigned long intervaloEnvio = 4000; // 4s
unsigned long ultimoTick = 0;
int contagem = 4;

// ================= MQ2 / MQ135 =================
#define MQ2_PIN 34
#define MQ135_PIN 35
const float R1 = 20000.0;
const float R2 = 10000.0;
const float ADC_MAX = 4095.0;
const float VCC = 3.3;
float R0_MQ2 = 0;
float R0_MQ135 = 0;
const int calibTime = 5000;

// ================= CCS811 =================
Adafruit_CCS811 ccs;

// ================= ULTRASSÔNICO =================
#define TRIG 14
#define ECHO 27
float distancia = 0;

// ================= UART ESP2 =================
#define RX2 16
#define TX2 17
String recebido = "";
float rx_temp = 0;
float rx_tds  = 0;
float rx_ph   = 0;
float rx_ntu  = 0;

// ================= LORA =================
#define LORA_SCK  18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS   5
#define LORA_RST  26
#define LORA_DIO0 2
#define LORA_FREQ 868E6

// ================= FUNÇÕES AUXILIARES =================
float readSensor(int pin) {
  int raw = analogRead(pin);
  float Vout = (raw / ADC_MAX) * VCC;
  return Vout * (R1 + R2) / R2;
}

float getRs(float Vin) {
  if (Vin <= 0) return 0;
  return ((VCC * R2 / Vin) - R2);
}

bool getCampo(String s, String chave, float &dest) {
  int i = s.indexOf(chave);
  if (i < 0) return false;
  i += chave.length();
  int f = s.indexOf(';', i);
  if (f < 0) f = s.length();
  dest = s.substring(i, f).toFloat();
  return true;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
  while (Serial2.available()) Serial2.read();

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("ERRO: LoRa não iniciou");
    while (1);
  }

  Wire.begin(21, 22);
  if (!ccs.begin()) {
    Serial.println("ERRO: CCS811 não encontrado");
    while (1);
  }
  ccs.setDriveMode(CCS811_DRIVE_MODE_1SEC);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);

  // Calibração MQ
  Serial.println("Calibrando MQ-2 e MQ-135...");
  unsigned long start = millis();
  float s2=0, s135=0; int n=0;
  while (millis()-start < calibTime) {
    s2 += getRs(readSensor(MQ2_PIN));
    s135 += getRs(readSensor(MQ135_PIN));
    n++;
    delay(100);
  }
  R0_MQ2 = s2/n;
  R0_MQ135 = s135/n;

  Serial.println("ESP1 MASTER pronto!");
}

// ================= LOOP =================
void loop() {
  // Sensores locais
  float Rs_MQ2   = getRs(readSensor(MQ2_PIN));
  float Rs_MQ135 = getRs(readSensor(MQ135_PIN));
  float ch4_pct  = constrain((1 - Rs_MQ2/R0_MQ2) * 100, 0, 100);
  float nh3_pct  = constrain((1 - Rs_MQ135/R0_MQ135) * 100, 0, 100);

  static float co2=0, tvoc=0;
  if (ccs.available() && !ccs.readData()) {
    co2 = ccs.geteCO2();
    tvoc = ccs.getTVOC();
  }

  // Ultrassônico
  digitalWrite(TRIG,HIGH); delayMicroseconds(10); digitalWrite(TRIG,LOW);
  long dur = pulseIn(ECHO,HIGH,30000);
  distancia = dur>0 ? dur*0.0343/2 : 0;

  // Envio a cada 4s
  if (millis()-ultimoEnvio >= intervaloEnvio) {
    ultimoEnvio = millis();
    contagem = 4;

    // Envia para ESP2
    String pacoteESP1 =
      "CH4:" + String(ch4_pct,0) + ";" +
      "NH3:" + String(nh3_pct,0) + ";" +
      "CO2:" + String(co2,0) + ";" +
      "TVOC:" + String(tvoc,0) + ";" +
      "DIST:" + String(distancia,1);

    Serial2.println(pacoteESP1);
    Serial.println("[ESP1] TX -> ESP2: " + pacoteESP1);
    Serial2.println("REQ");

    // Aguarda resposta ESP2
    String respostaESP2=""; 
    unsigned long t0=millis();
    while (millis()-t0<1000) {
      if (Serial2.available()) {
        respostaESP2 = Serial2.readStringUntil('\n');
        respostaESP2.trim();
        break;
      }
    }

    if (respostaESP2.length()==0) {
      Serial.println("⚠ ESP2 não respondeu");
      return;
    }

    // Parse ESP2
    getCampo(respostaESP2,"TEMP:",rx_temp);
    getCampo(respostaESP2,"TDS:", rx_tds);
    getCampo(respostaESP2,"NTU:", rx_ntu);
    getCampo(respostaESP2,"PH:", rx_ph);

    // Serial debug
    Serial.println("\n===== ESP1 (LOCAL) =====");
    Serial.printf("CH4: %.0f%% | NH3: %.0f%% | CO2: %.0f | TVOC: %.0f | DIST: %.1f\n",
                  ch4_pct, nh3_pct, co2, tvoc, distancia);
    Serial.println("===== ESP2 =====");
    Serial.printf("TEMP: %.2f | TDS: %.0f | NTU: %.0f | PH: %.2f\n",
                  rx_temp, rx_tds, rx_ntu, rx_ph);

    // Envia LoRa
    String pacoteLoRa =
      "TEMP:" + String(rx_temp,2) + ";" +
      "TDS:"  + String(rx_tds,0)  + ";" +
      "NTU:"  + String(rx_ntu,0)  + ";" +
      "PH:"   + String(rx_ph,2)   + ";" +
      "CH4:"  + String(ch4_pct,0) + ";" +
      "NH3:"  + String(nh3_pct,0) + ";" +
      "CO2:"  + String(co2,0)     + ";" +
      "DIST:" + String(distancia,1);

    LoRa.beginPacket();
    LoRa.print(pacoteLoRa);
    LoRa.endPacket();
    Serial.println("📡 LoRa enviado: " + pacoteLoRa);
  }
}
