// =====================================================
// UART MASTER + LoRa TX - ESP1
// =====================================================

#include <Wire.h>
#include "Adafruit_CCS811.h"
#include <SPI.h>
#include <LoRa.h>

// =============== delay
unsigned long ultimoEnvio = 0;
const unsigned long intervaloEnvio = 4000; // 4 segundos

int contagem = 4;
unsigned long ultimoTick = 0;


// ---------------- MQ2 / MQ135 ----------------
#define MQ2_PIN 34
#define MQ135_PIN 35

// Constantes para cálculo de tensão e resistência

const float R1 = 20000.0;
const float R2 = 10000.0;
const float ADC_MAX = 4095.0;
const float VCC = 3.3;

float R0_MQ2 = 0;
float R0_MQ135 = 0;
const int calibTime = 5000;

// ---------------- CCS811 ----------------
Adafruit_CCS811 ccs;

// ---------------- Ultrassônico ----------------
#define TRIG 14
#define ECHO 27

// ---------------- UART2 ----------------
#define RX2 16
#define TX2 17

String recebido = "";

// Dados recebidos do ESP2 

float rx_temp = 0;
float rx_tds  = 0;
float rx_ph   = 0;
float rx_ntu  = 0;

// ---------------- LoRa ----------------
#define LORA_SCK  18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS   5
#define LORA_RST  26
#define LORA_DIO0 2

// Frequência
#define LORA_FREQ 868E6

// =====================================================
// FUNÇÕES AUXILIARES
// =====================================================

//Lê tensão do sensor
float readSensor(int pin) {
  int raw = analogRead(pin);
  float Vout = (raw / ADC_MAX) * VCC;
  float Vin = Vout * (R1 + R2) / R2;
  return Vin;
}
//Calcula Rs do sensor
float getRs(float Vin) {
  if (Vin <= 0) return 0;
  return ((VCC * R2 / Vin) - R2);
}

// curva log–log genérica (datasheet)
float mq_ppm(float ratio, float m, float b) {
  return pow(10, (m * log10(ratio) + b));
}

//extrai valor de chave em string 
float getValor(String texto, String chave);

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);

  //inicializa SPI e Lora 
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("Erro ao iniciar LoRa!");
    while (true); // Para aqui se não inicializar
  }
  Serial.println("LoRa inicializado com sucesso!");

  // ---- Calibração MQ ------------------------------------------------------
  Serial.println("Calibrando MQ-2 e MQ-135...");
  unsigned long start = millis();
  float sum_MQ2 = 0, sum_MQ135 = 0;
  int count = 0;

  while (millis() - start < calibTime) {
    float Rs2 = getRs(readSensor(MQ2_PIN));
    float Rs135 = getRs(readSensor(MQ135_PIN));
    if (Rs2 > 0) sum_MQ2 += Rs2;
    if (Rs135 > 0) sum_MQ135 += Rs135;
    count++;
    delay(100);
  }

  R0_MQ2 = sum_MQ2 / count;
  R0_MQ135 = sum_MQ135 / count;

  Serial.print("R0_MQ2 = "); Serial.print(R0_MQ2, 2);
  Serial.print(" | R0_MQ135 = "); Serial.println(R0_MQ135, 2);

  // ---- CCS811 ----------------------------------------------------------
  Wire.begin(21, 22);
  if (!ccs.begin()) {
    Serial.println("ERRO: CCS811 não encontrado");
    while (1);
  }
  ccs.setDriveMode(CCS811_DRIVE_MODE_1SEC);

  // ---- Ultrassônico ----------------------------------------------------
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);

  Serial.println("Setup concluído!");
  Serial.println("ESP1 MASTER Pronto");
}

// ====================== LOOP ======================
void loop() {

  // -------- MQ --------------------------------------------
  float Rs_MQ2   = getRs(readSensor(MQ2_PIN));
  float Rs_MQ135 = getRs(readSensor(MQ135_PIN));

  float ratio_MQ2   = Rs_MQ2 / R0_MQ2;
  float ratio_MQ135 = Rs_MQ135 / R0_MQ135;

  bool alerta_CH4 = (ratio_MQ2 < 0.6);
  bool alerta_NH3 = (ratio_MQ135 < 0.6);

  float ch4_ppm = mq_ppm(ratio_MQ2, -2.7, 1.8);
  float nh3_ppm = mq_ppm(ratio_MQ135, -1.85, 1.3);
  
  float ch4_pct = constrain((1.0 - ratio_MQ2) * 100.0, 0, 100);
  float nh3_pct = constrain((1.0 - ratio_MQ135) * 100.0, 0, 100);


  // -------- CCS811 ----------------------------------------------------
  static float co2 = 0, tvoc = 0;
  if (ccs.available()) {
    if (!ccs.readData()) {
      co2 = ccs.geteCO2();
      tvoc = ccs.getTVOC();
    }
  }

  // -------- Ultrassônico --------------------------------------------------
  digitalWrite(TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duracao = pulseIn(ECHO, HIGH, 30000);
  float distancia = (duracao > 0) ? duracao * 0.0343 / 2 : 0;

  // -------- SERIAL (DEBUG) --------
  // Serial.println("===== LEITURAS =====");
  // Serial.print("CO2: "); Serial.print(co2); Serial.println(" ppm");
  // Serial.print("TVOC: "); Serial.print(tvoc); Serial.println(" ppb");
  // Serial.print("CH4: "); Serial.println(alerta_CH4 ? "ALERTA" : "OK");
  // Serial.print("NH3: "); Serial.println(alerta_NH3 ? "ALERTA" : "OK");
  // Serial.print("MQ2 Rs/R0: "); Serial.println(ratio_MQ2, 3);
  // Serial.print("MQ135 Rs/R0: "); Serial.println(ratio_MQ135, 3);
  // Serial.print("CH4 est.: "); Serial.print(ch4_ppm); Serial.println(" ppm");
  // Serial.print("NH3 est.: "); Serial.print(nh3_ppm); Serial.println(" ppm");
  // Serial.print("Distancia: "); Serial.print(distancia,1); Serial.println(" cm");
  // Serial.println("====================\n");
  Serial.println("===== LEITURAS ESP2 =====");
  Serial.printf("CO2: %.0f ppm\n", co2);
  Serial.printf("TVOC: %.0f ppb\n", tvoc);
  Serial.printf("CH4: %s\n", alerta_CH4 ? "ALERTA" : "OK");
  Serial.printf("NH3: %s\n", alerta_NH3 ? "ALERTA" : "OK");
  Serial.printf("MQ2 Rs/R0: %.3f\n", ratio_MQ2);
  Serial.printf("MQ135 Rs/R0: %.3f\n", ratio_MQ135);
  Serial.printf("CH4 est.: %.2f ppm\n", ch4_ppm);
  Serial.printf("NH3 est.: %.2f ppm\n", nh3_ppm);
  Serial.printf("Distancia: %.1f cm\n", distancia);
  Serial.println("====================\n");

  //================ UART COM ESP 2 ==============================

  if (millis() - ultimoEnvio >= intervaloEnvio) {
  ultimoEnvio = millis();
  contagem = 4;
  
  while (Serial2.available()) Serial2.read(); //LIMPA BUFFER
  Serial2.println("REQ"); // solicita dados do esp 2

  // Aguarda resposta com timeout
  unsigned long t0 = millis();
  while (!Serial2.available()) {
    if (millis() - t0 > 500) {
      Serial.println("Timeout ESP2");
      break;
    }
  }
  //RECEBE DO ESP2
  if (Serial2.available()) {
    recebido = Serial2.readStringUntil('\n');
    recebido.trim();

    rx_temp = getValor(recebido, "Temp:");
    rx_tds  = getValor(recebido, "TDS:");
    rx_ntu  = getValor(recebido, "NTU:");
    rx_ph   = getValor(recebido, "PH:");
 
    Serial.println("=== DADOS ESP2 ===");
    Serial.print("Temp: "); Serial.println(rx_temp);
    Serial.print("TDS : "); Serial.println(rx_tds);
    Serial.print("Turb: "); Serial.println(rx_ntu);
    Serial.print("pH  : "); Serial.println(rx_ph);
  }

 //ENVIA UART ======================================================

 if (millis() - ultimoTick >= 1000) {
  ultimoTick = millis();
  Serial.print("Enviando em ");
  Serial.print(contagem);
  Serial.println("...");
  contagem--;

  if (contagem < 0) contagem = 4;
}

 // -------- UART ORGANIZADO ENVIA --------
 // OK = 1 | ALERTA = 0
 String pacoteESP1 =
  "CH4_OK:" + String(alerta_CH4 ? 0 : 1) + "," +
  "CH4_P:"  + String(ch4_pct, 0) + "," +
  "NH3_OK:" + String(alerta_NH3 ? 0 : 1) + "," +
  "NH3_P:"  + String(nh3_pct, 0) + "," +
  "CO2:"    + String(co2, 0) + "," +
  "TVOC:"   + String(tvoc, 0) + "," +
  "DIST:"   + String(distancia, 1);

 Serial2.println(pacoteESP1);// envia pacote para o esp2

//======= envio lora 
 String pacoteLoRa = "DADOSMIMOSA," +
    String(rx_temp,1) + "," +
    String(rx_tds,0)  + "," +
    String(rx_ntu,0)  + "," +
    String(rx_ph,2)   + "," +
    String(ch4_pct,0) + "," +
    String(nh3_pct,0) + "," +
    String(co2,0)     + "," +
    String(distancia,1);

  LoRa.beginPacket();
  LoRa.print(pacoteLoRa);
  LoRa.endPacket();

  Serial.println("LoRa enviado:");
  Serial.println(pacoteLoRa);

  delay(2000);
}

// ====================================
float getValor(String texto, String chave) {
  int i = texto.indexOf(chave);
  if (i == -1) return 0;
  i += chave.length();
  int f = texto.indexOf(",", i);
  if (f == -1) f = texto.length();
  return texto.substring(i, f).toFloat();
}


