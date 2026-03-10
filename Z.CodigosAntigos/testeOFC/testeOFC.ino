/*
  ESP32 + MQ-2 + MQ-135 (ambos analógicos)
  - Leitura simultânea com média (reduz ruído)
  - Mostra: ADC bruto, tensão no pino (já dividida), tensão estimada no AO do sensor (antes do divisor)
  - Divisor: Rtop = 20k (sensor -> nó), Rbottom = 10k (nó -> GND)
*/

//esp de cima usar esse codigo 


#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_CCS811.h"
#include <SPI.h>
#include <LoRa.h>


Adafruit_CCS811 ccs;
float last_co2_percent = 0.0f;


// ===== PINOS ADC1 (recomendados) =====
static const int PIN_MQ2   = 34; // MQ-2  (metano-ish)
static const int PIN_MQ135 = 35; // MQ-135 (amonia-ish)

// ===== PINOS (conforme você pediu) =====
static const int TRIG_PIN = 27;
static const int ECHO_PIN = 32; 

// ===== PARAMETROS DO BALDE (cilindro) =====
// Ajuste depois conforme seu balde real:
static const float BUCKET_HEIGHT_CM   = 20.0f;  // altura interna útil (cm)
static const float BUCKET_DIAMETER_CM = 18.0f;  // diâmetro interno (cm)
// Obs: 18cm x 20cm dá ~5.1 L (bem próximo de 5L)

// Distância "extra" (cm) caso o sensor não esteja exatamente no topo interno.
// Se o sensor estiver colado por cima e "enxerga" um pouco antes do nível interno, ajuste aqui.
// Comece com 0 e ajuste depois se precisar.
static const float SENSOR_OFFSET_CM = 0.0f;

// ===== CONFIG MEDIÇÃO =====
static const uint32_t TIMEOUT_US = 25000; // ~25ms (até ~4m), evita travar
static const float SPEED_CM_PER_US = 0.0343f; // velocidade do som ~343 m/s => 0.0343 cm/us

//sensor roxo co2
static const int I2C_SDA = 21;
static const int I2C_SCL = 22;

// Faixa de referencia do CO2 equivalente
static const float CO2_MIN_PPM = 400.0f;   // ar limpo
static const float CO2_MAX_PPM = 2000.0f;  // ar ruim

// ===== DIVISOR DE TENSÃO =====
// static const float R_TOP    = 20000.0f; // 20k (do AO do sensor até o nó)
// static const float R_BOTTOM = 10000.0f; // 10k (do nó até GND)

// Fator pra reconstruir a tensão original do sensor (antes do divisor)
// V_sensor = V_pin * (Rtop + Rbottom) / Rbottom
// static const float DIV_GAIN = (R_TOP + R_BOTTOM) / R_BOTTOM; // (20k+10k)/10k = 3.0

// ===== CONFIG ADC ESP32 =====
// 12 bits -> 0..4095
static const int ADC_MAX = 4095;

// Converte ADC -> Volts no pino (aprox).
// Com ADC_11db, o range útil vai perto de ~3.3V (não é perfeito, mas serve bem pra debug).
// float adcToVolts(uint16_t adc) {
//   return ( (float)adc / (float)ADC_MAX ) * 3.3f;
// }

// ===== CONTROLE DE ENVIO =====
unsigned long ultimoEnvio = 0;
const unsigned long intervaloEnvio = 3000; // 3s

// ===== IDS (ajuste conforme seu projeto) =====
const int TANQUE_ID = 5;
const int REGIAO_ID = 1;

// ===== DADOS RECEBIDOS DO ESP2 (precisa existir mesmo que venha vazio) =====
float rx_temp = 0.0f;
float rx_ph = 0.0f;
int   rx_tds_cond = 0;
int   rx_turbidez = 0;


// ================= UART ESP2 =================
#define RX2 16
#define TX2 17

// ================= LORA =================
#define LORA_SCK  18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS   5
#define LORA_RST  26
#define LORA_DIO0 33
#define LORA_FREQ 915E6

uint16_t readADC_Avg(int pin, int samples = 20, int delayMs = 5) {
  uint32_t sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(delayMs);
  }
  return (uint16_t)(sum / samples);
}

float measureDistanceCm() {
  // pulso TRIG
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  uint32_t duration = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);
  if (duration == 0) return -1.0f;

  return (duration * SPEED_CM_PER_US) / 2.0f;
}

bool getCampo(String s, const String &chave, float &dest) {
  int i = s.indexOf(chave);
  if (i < 0) return false;
  i += chave.length();
  int j = s.indexOf(';', i);
  if (j < 0) j = s.length();
  dest = s.substring(i, j).toFloat();
  return true;
}

bool getCampo(String s, const String &chave, int &dest) {
  int i = s.indexOf(chave);
  if (i < 0) return false;
  i += chave.length();
  int j = s.indexOf(';', i);
  if (j < 0) j = s.length();
  dest = s.substring(i, j).toInt();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("BOOT: entrou no setup()");


  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
  while (Serial2.available()) Serial2.read();

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
  Serial.println("ERRO: LoRa nao iniciou (verifique alimentacao/pinos/frequencia)");
  while (1) { delay(1000); }
  }


  Wire.begin(I2C_SDA, I2C_SCL);
  if (!ccs.begin(0x5A)) {
    Serial.println("Erro CCS811");
    while (1) delay(10);
  }

  // Recomendo fixar resolução e atenuação
  analogReadResolution(12); // 0..4095
  analogSetPinAttenuation(PIN_MQ2, ADC_11db);
  analogSetPinAttenuation(PIN_MQ135, ADC_11db);

  //ultrassonico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // garante TRIG em LOW
  digitalWrite(TRIG_PIN, LOW);

  // // Obs: ADC1 não conflita com Wi-Fi (ADC2 conflita)
  // Serial.println("\n=== MQ-2 + MQ-135 (leituras analogicas) ===");
  // Serial.println("Aqueça os sensores por pelo menos 1-3 minutos para leituras mais estaveis.");
  // Serial.println("Divisor: 20k em cima (sensor) e 10k em baixo (GND).");

  Serial.println("ESP1 MASTER pronto!");
}

void loop() {

  //gases
  // Média das leituras
  uint16_t mq2_adc   = readADC_Avg(PIN_MQ2,   25, 4);
  uint16_t mq135_adc = readADC_Avg(PIN_MQ135, 25, 4);

  // Normalização simples (0.0 a 1.0)
  float metano = (float)mq2_adc / (float)ADC_MAX;
  float amonia = (float)mq135_adc / (float)ADC_MAX;

  // porcentagens precisam existir fora do IF também
  float metano_pct = metano * 100.0f;
  float amonia_pct = amonia * 100.0f;

  //ultrassonico
  float distance_cm = measureDistanceCm();
  float volume_L = 0.0f;

  if (distance_cm >= 0) {
    distance_cm -= SENSOR_OFFSET_CM;

    float liquid_height_cm = BUCKET_HEIGHT_CM - distance_cm;
    if (liquid_height_cm < 0) liquid_height_cm = 0;
    if (liquid_height_cm > BUCKET_HEIGHT_CM) liquid_height_cm = BUCKET_HEIGHT_CM;

    float radius_cm = BUCKET_DIAMETER_CM / 2.0f;
    float area_cm2  = PI * radius_cm * radius_cm;

    float volume_cm3 = area_cm2 * liquid_height_cm;
    volume_L = volume_cm3 / 1000.0f;
  }

  // ===== CCS811 (CO2%) =====
  if (ccs.available()) {
    if (!ccs.readData()) {
      uint16_t co2_ppm = ccs.geteCO2();

      float co2_percent =
        ((float)co2_ppm - CO2_MIN_PPM) /
        (CO2_MAX_PPM - CO2_MIN_PPM) * 100.0f;

      if (co2_percent < 0)   co2_percent = 0;
      if (co2_percent > 100) co2_percent = 100;

      last_co2_percent = co2_percent;
    }
  }
  //envio rx tx
  if (millis()-ultimoEnvio >= intervaloEnvio) {
    ultimoEnvio = millis();

    while (Serial2.available()) Serial2.read();

     //porcentagem
    // float metano_pct   = metano * 100.0f;
    // float amonia_pct   = amonia * 100.0f;
    float co2          = last_co2_percent; // aqui é o CO2% (equivalente)
    float nivel_litros = volume_L;

    // Envia para ESP2
    String pacoteESP1 =
      "metano:" + String(metano_pct,0) + ";" +
      "amonia:" + String(amonia_pct,0) + ";" +
      "co2:" + String(co2,0) + ";" +
      "nivel:" + String(nivel_litros,1);

    Serial2.println(pacoteESP1);
    Serial.println("[ESP1] TX -> ESP2: " + pacoteESP1);

    //solicita resposta
    Serial2.println("REQ");

    // Aguarda resposta ESP2
    String respostaESP2 = "";
    unsigned long t0 = millis();

    while (millis() - t0 < 1500) { // 1.5s
      if (Serial2.available()) {
        String linha = Serial2.readStringUntil('\n');
        linha.trim();

        // ignora qualquer coisa que não seja resposta de sensor
        if (linha.startsWith("temp:")) {
          respostaESP2 = linha;
          break;
        }
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

   // ===== PRINT 1 LINHA (ideal pra LoRa/MQTT) =====
  Serial.print("co2:");
  Serial.print(last_co2_percent, 1);
  Serial.print(";metano:");
  Serial.print(metano_pct, 2);
  Serial.print(";amonia:");
  Serial.print(amonia_pct, 2);
  Serial.print(";volume:");
  Serial.println(volume_L, 2);

  delay(1000);
}
