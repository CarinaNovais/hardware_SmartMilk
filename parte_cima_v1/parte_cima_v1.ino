#include <Wire.h>
#include "Adafruit_CCS811.h"

// ---------------- MQ2 / MQ135 ----------------
#define MQ2_PIN 34
#define MQ135_PIN 35

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

// UART2
#define RX2 16
#define TX2 17

// RECEBIMENTO UART2
String recebido = "";

// Variáveis recebidas DO OUTRO ESP
float rx_temp = 0, rx_tds = 0, rx_ntu = 0, rx_ph = 0;

// ====================== Funções MQ ======================
float readSensor(int pin) {
  int raw = analogRead(pin);
  float Vout = (raw / ADC_MAX) * VCC;
  float Vin = Vout * (R1 + R2) / R2;
  return Vin;
}

float getRs(float Vin) {
  if (Vin <= 0) return 0;
  return ((VCC * R2 / Vin) - R2);
}

float getGasPercentage(float Rs, float R0) {
  float ratio = Rs / R0;
  float A = 100.0;
  float B = -1.2;
  float perc = A * pow(ratio, B);
  if (perc < 0) perc = 0;
  if (perc > 100) perc = 100;
  return perc;
}

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("Receptor iniciado...");

  delay(2000);

  // ---- MQ2 / MQ135 ----
  Serial.println("Calibrando sensores MQ-2 e MQ-135...");
  unsigned long start = millis();
  float sum_MQ2 = 0, sum_MQ135 = 0;
  int count = 0;

  while (millis() - start < calibTime) {
    float Rs_MQ2 = getRs(readSensor(MQ2_PIN));
    float Rs_MQ135 = getRs(readSensor(MQ135_PIN));
    if (Rs_MQ2 > 0) sum_MQ2 += Rs_MQ2;
    if (Rs_MQ135 > 0) sum_MQ135 += Rs_MQ135;
    count++;
    delay(100);
  }

  R0_MQ2 = sum_MQ2 / count;
  R0_MQ135 = sum_MQ135 / count;
  Serial.print("R0_MQ2 = "); Serial.print(R0_MQ2, 2);
  Serial.print(" | R0_MQ135 = "); Serial.println(R0_MQ135, 2);

  // ---- CCS811 ----
  Wire.begin(21, 22);

  if (!ccs.begin()) {
    Serial.println("ERRO: Falha ao iniciar CCS811!");
    while (1);
  }
  delay(2000);
  ccs.setDriveMode(CCS811_DRIVE_MODE_1SEC);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);

  Serial.println("Setup concluído!");
}

// ====================== LOOP ======================
void loop() {

  // ---- MQ2 / MQ135 ----
  float Rs_MQ2 = getRs(readSensor(MQ2_PIN));
  float Rs_MQ135 = getRs(readSensor(MQ135_PIN));

  float ratio_MQ2   = Rs_MQ2 / R0_MQ2;
  float ratio_MQ135 = Rs_MQ135 / R0_MQ135;

  bool alerta_CH4  = (ratio_MQ2 < 0.6);
  bool alerta_NH3  = (ratio_MQ135 < 0.6);


  float perc_CH4 = getGasPercentage(Rs_MQ2, R0_MQ2);
  float perc_NH3 = getGasPercentage(Rs_MQ135, R0_MQ135);


log(ppm) = m * log(Rs/R0) + b
float mq_ppm(float ratio, float m, float b) {
  return pow(10, (m * log10(ratio) + b));
}
float ch4_ppm = mq_ppm(ratio_MQ2, -2.7, 1.8);
float nh3_ppm = mq_ppm(ratio_MQ135, -1.85, 1.3);



  // ---- CCS811 ----roxo
static float co2 = 0, tvoc = 0;

if (ccs.available()) {
  if (!ccs.readData()) {
    co2 = ccs.geteCO2();
    tvoc = ccs.getTVOC();
  }
}

  // ---- Ultrassônico ----
  digitalWrite(TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duracao = pulseIn(ECHO, HIGH, 30000);
  float distancia = 0;

  if (duracao > 0)
      distancia = duracao * 0.0343 / 2;

    // ---------------- SERIAL MONITOR ----------------
Serial.println("===== LEITURAS ATUAIS =====");

Serial.print("MQ2 (CH4): ");
Serial.print(perc_CH4, 1);
Serial.println(" %");

Serial.print("MQ135 (NH3): ");
Serial.print(perc_NH3, 1);
Serial.println(" %");

Serial.print("CH4: ");
Serial.println(alerta_CH4 ? "ALERTA" : "NORMAL");

Serial.print("NH3: ");
Serial.println(alerta_NH3 ? "ALERTA" : "NORMAL");

Serial.print("MQ2 Rs/R0: ");
Serial.println(ratio_MQ2, 3);

Serial.print("MQ135 Rs/R0: ");
Serial.println(ratio_MQ135, 3);

Serial.print("CH4 estimado: ");
Serial.print(ch4_ppm);
Serial.println(" ppm");

Serial.print("NH3 estimado: ");
Serial.print(nh3_ppm);
Serial.println(" ppm");


Serial.print("CCS811 CO2: ");
Serial.print(co2);
Serial.println(" ppm");

Serial.print("CCS811 TVOC: ");
Serial.print(tvoc);
Serial.println(" ppb");

Serial.print("Distancia: ");
Serial.print(distancia, 1);
Serial.println(" cm");

Serial.println("===========================\n");


  // ---------------- ENVIO UART2 ----------------
  String pacote =
    "CH4:" + String(perc_CH4, 1) + "," +
    "NH3:" + String(perc_NH3, 1) + "," +
    "CO2:" + String(co2) + "," +
    "TVOC:" + String(tvoc) + "," +
    "DIST:" + String(distancia, 1);

  Serial2.println(pacote);

  // ---------------- RECEBIMENTO UART2 ----------------
  if (Serial2.available()) {

    recebido = Serial2.readStringUntil('\n');
    Serial.print("Recebido: ");
    Serial.println(recebido);

    rx_temp = getValor(recebido, "Temp:");
    rx_tds = getValor(recebido, "TDS:");
    rx_ntu = getValor(recebido, "NTU:");
    rx_ph = getValor(recebido, "PH:");
    
    Serial.print("TEMP = "); Serial.println(rx_temp);
    Serial.print("TDS  = "); Serial.println(rx_tds);
    Serial.print("NTU  = "); Serial.println(rx_ntu);
    Serial.print("PH   = "); Serial.println(rx_ph);
  }

  delay(800);
}

// ===============================
// FUNÇÃO PARA PEGAR O VALOR
// ===============================
float getValor(String texto, String chave) {
  int ini = texto.indexOf(chave);
  if (ini == -1) return 0;

  ini += chave.length();
  int fim = texto.indexOf(",", ini);
  if (fim == -1) fim = texto.length();

  return texto.substring(ini, fim).toFloat();
}
