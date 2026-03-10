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

//RX2 TX2
#define RX2 16
#define TX2 17

// RECEBENDO 
String recebido = "";

// Variáveis recebidas
float rx_nome = 0, rx_nome = 0, rx_nome = 0, rx_nome = 0, rx_nome = 0;

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
  Serial.println("Calibrando sensores MQ-2 e MQ-135 em ar limpo...");
  unsigned long start = millis();
  float sum_MQ2 = 0, sum_MQ135 = 0;
  int count = 0;
  while (millis() - start < calibTime) {
    float Rs_MQ2 = getRs(readSensor(MQ2_PIN));
    float Rs_MQ135 = getRs(readSensor(MQ135_PIN));
    if (Rs_MQ2 > 0) sum_MQ2 += Rs_MQ2;
    if (Rs_MQ135 > 0) sum_MQ135 += Rs_MQ135;
    count++;
    delay(200);
  }
  R0_MQ2 = sum_MQ2 / count;
  R0_MQ135 = sum_MQ135 / count;
  Serial.print("R0_MQ2 = "); Serial.print(R0_MQ2, 2);
  Serial.print(" Ω | R0_MQ135 = "); Serial.println(R0_MQ135, 2);

  // ---- CCS811 ----
  Wire.begin(21, 22);
  if (!ccs.begin()) {
    Serial.println("Erro ao iniciar CCS811!");
    while (1);
  }
  Serial.println("CCS811 encontrado, aguardando estabilizar...");
  while (!ccs.available());

  // ---- Ultrassônico ----
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
  float perc_CH4 = (R0_MQ2 > 0 && Rs_MQ2 > 0) ? getGasPercentage(Rs_MQ2, R0_MQ2) : 0;
  float perc_NH3 = (R0_MQ135 > 0 && Rs_MQ135 > 0) ? getGasPercentage(Rs_MQ135, R0_MQ135) : 0;

  Serial.print("CH4: "); Serial.print(perc_CH4, 1); Serial.print("% | ");
  Serial.print("NH3: "); Serial.print(perc_NH3, 1); Serial.println("%");
  if (perc_CH4 > 10) Serial.println("⚠️ Alerta CH4!");
  if (perc_NH3 > 5) Serial.println("⚠️ Alerta NH3!");

  // ---- CCS811 ----
  if (ccs.available()) {
    if (!ccs.readData()) {
      Serial.print("CO2: "); Serial.print(ccs.geteCO2()); Serial.print(" ppm | ");
      Serial.print("TVOC: "); Serial.print(ccs.getTVOC()); Serial.println(" ppb");
    } else {
      Serial.println("Erro ao ler CCS811!");
    }
  }

  // ---- Ultrassônico ----
  digitalWrite(TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duracao = pulseIn(ECHO, HIGH, 30000);
  if (duracao == 0) {
    Serial.println("ERRO: ECHO sem pulso");
  } else {
    float distancia = duracao * 0.0343 / 2;
    if (distancia < 2 || distancia > 450) {
      Serial.print("Fora do alcance: "); Serial.print(distancia); Serial.println(" cm");
    } else {
      Serial.print("Distancia: "); Serial.print(distancia); Serial.println(" cm");
    }
  }

  Serial.println("-----------------------------");
  delay(1000);
// ---------------- ENVIO UART2  PARA O OUTRO ESP----------------
String pacote =
  "CH4:" + String(perc_CH4, 1) + "," +
  "NH3:" + String(perc_NH3, 1) + "," +
  "CO2:" + String(ccs.geteCO2()) + "," +
  "TVOC:" + String(ccs.getTVOC()) + "," +
  "DIST:" + String(distancia, 1);

Serial2.println(pacote);   // envia para outro ESP32

// --------RECEBENDO DO OUTRO ESP 

  if (Serial2.available()) {

    recebido = Serial2.readStringUntil('\n');
    Serial.print("Recebido: ");
    Serial.println(recebido);

    rx_nome  = getValor(recebido, "nome:");
    rx_nome  = getValor(recebido, "nome:");
    rx_nome  = getValor(recebido, "nome:");
    rx_nome = getValor(recebido, "nome:");
    rx_nome = getValor(recebido, "nome:");

    Serial.print("nome -> ");  Serial.println(rx_nome);
    Serial.print("nome -> ");  Serial.println(rx_nome);
    Serial.print("nome -> ");  Serial.println(rx_nome);
    Serial.print("nome -> "); Serial.println(rx_nome);
    Serial.print("nome -> "); Serial.println(rx_nome);
    Serial.println("------------------------");
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

  String valor = texto.substring(ini, fim);
  return valor.toFloat();
}

