// =====================================================
// ESP1 - UART MASTER + LoRa TX (valores apenas)
// =====================================================

#include <Wire.h>
#include "Adafruit_CCS811.h"
#include <SPI.h>
#include <LoRa.h>

//identificacao tanquw
#define TANQUE_ID 5 
#define REGIAO_ID 1

// ================= CONTROLE DE TEMPO =================
unsigned long ultimoEnvio = 0;
const unsigned long intervaloEnvio = 4000; // 4s

//unsigned long ultimoTick = 0;
//int contagem = 4;

// ================= MQ2 / MQ135 =================
#define MQ2_PIN 34
#define MQ135_PIN 35
float metano_pct = 0, amonia_pct = 0;
float distancia_valida = 25.0f;
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
#define TRIG 25
#define ECHO 32
float distancia = 0;

//
const float ALTURA_TANQUE_CM = 25.0;       // altura interna total do balde
const float DIAMETRO_TANQUE_CM = 30.0;     //(diâmetro interno do balde)

// ================= UART ESP2 =================
#define RX2 16
#define TX2 17

//String recebido = "";

// dados q vem do esp 2 
float rx_temp = 0;
float rx_tds_cond  = 0;
float rx_ph   = 0;
float rx_turbidez  = 0;

// ================= LORA =================
#define LORA_SCK  18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS   5
#define LORA_RST  26
#define LORA_DIO0 33
#define LORA_FREQ 915E6

// ================= FUNÇÕES AUXILIARES =================
float readSensor(int pin) {
  int raw = analogRead(pin);
  float Vout = (raw / ADC_MAX) * VCC;
  return Vout;
  //return Vout * (R1 + R2) / R2;
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

float nivelCmFromDist(float dist_cm, float altura_cm) {
  if (dist_cm <= 0) return 0;
  float h = altura_cm - dist_cm;
  if (h < 0) h = 0;
  if (h > altura_cm) h = altura_cm;
  return h;
}

float litrosFromNivel(float nivel_cm, float diametro_cm) {
  float r = diametro_cm / 2.0;
  float volume_cm3 = 3.14159265 * r * r * nivel_cm;
  return volume_cm3 / 1000.0; // cm³ -> litros
}

float litrosFromDist(float dist_cm, float altura_cm, float diametro_cm) {
  float nivel_cm = nivelCmFromDist(dist_cm, altura_cm);
  return litrosFromNivel(nivel_cm, diametro_cm);
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

  if (R0_MQ2 > 0) metano_pct  = constrain((1 - Rs_MQ2/R0_MQ2) * 100, 0, 100);
  if (R0_MQ135 > 0) amonia_pct  = constrain((1 - Rs_MQ135/R0_MQ135) * 100, 0, 100);

 static float co2 = 0;
  if (ccs.available() && !ccs.readData()) {
    co2 = ccs.geteCO2();  // CO2 (eCO2) do CCS811
  }

  
  // ultrassonico (distancia do sensor ate a superficie)
  // ultrassonico
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long dur = pulseIn(ECHO, HIGH, 30000);
  float d = (dur > 0) ? (dur * 0.0343f / 2.0f) : 0.0f;

  if (dur > 0 && d >= 25.0f && d <= 250.0f) {
    distancia_valida = d;
  }
  distancia = distancia_valida;

  // CALIBRAÇÃO: distância medida quando o tanque está vazio
  const float DIST_VAZIO_CM = 25.0f; 
  float nivel_cm = DIST_VAZIO_CM - distancia;
  if (nivel_cm < 0) nivel_cm = 0;
  if (nivel_cm > DIST_VAZIO_CM) nivel_cm = DIST_VAZIO_CM;


  // litros (cilindro)
  float nivel_litros = litrosFromNivel(nivel_cm, DIAMETRO_TANQUE_CM);
  Serial.printf("US: dur=%ld us | d=%.1f cm | dist_valida=%.1f cm | nivel_cm=%.1f\n",
              dur, d, distancia_valida, nivel_cm);



  // envio a cada 4s
  if (millis()-ultimoEnvio >= intervaloEnvio) {
    ultimoEnvio = millis();

    // Envia para ESP2
    String pacoteESP1 =
      "metano:" + String(metano_pct,0) + ";" +
      "amonia:" + String(amonia_pct,0) + ";" +
      "co2:" + String(co2,0) + ";" +
      "nivel:" + String(nivel_litros,1);

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
      //return;
    }

    // Parse ESP2
    getCampo(respostaESP2,"temp:",rx_temp);
    getCampo(respostaESP2,"tds_cond:", rx_tds_cond);
    getCampo(respostaESP2,"turbidez:", rx_turbidez);
    getCampo(respostaESP2,"ph:", rx_ph);

    // Serial debug
    Serial.println("\n===== ESP1 (LOCAL) =====");
    Serial.printf("metano: %.0f%% | amonia: %.0f%% | co2: %.0f  | nivel: %.1f L\n", 
    metano_pct, amonia_pct, co2, nivel_litros);

    Serial.println("===== ESP2 =====");
    Serial.printf("temp: %.2f | tds_cond: %.0f | turbidez: %.0f | PH: %.2f\n",
                  rx_temp, rx_tds_cond, rx_turbidez, rx_ph);

    // Envia LoRa
    String pacoteLoRa =
      "tanque:" + String(TANQUE_ID)+";"+
      "regiao:" + String(REGIAO_ID) + ";" +
      "temp:" + String(rx_temp,2) + ";" +
      "condutividade:"  + String(rx_tds_cond,0)  + ";" +
      "turbidez:"  + String(rx_turbidez,0)  + ";" +
      "ph:"   + String(rx_ph,2)   + ";" +
      "metano:"  + String(metano_pct,0) + ";" +
      "amonia:"  + String(amonia_pct,0) + ";" +
      "co2:"  + String(co2,0)     + ";" +
      "nivel:" + String(nivel_litros,1);

    LoRa.beginPacket();
    LoRa.print(pacoteLoRa);
    LoRa.endPacket();
    Serial.println("📡 LoRa enviado: " + pacoteLoRa);
  }
}
