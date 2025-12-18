// =====================================================
// ESP 2 - LCD + Sensores locais (SLAVE)
// UART com ESP1 | DEBUG COMPLETO a cada 4s
// =====================================================

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>

// ================= LCD =================
U8G2_ST7920_128X64_F_SW_SPI u8g2(
  U8G2_R0,
  18,  // CLK
  4,   // MOSI
  5,   // CS
  26   // RST
);

// ================= ADS1115 =================
Adafruit_ADS1115 ads;
const float mvPorBit      = 0.1875;
const float FATOR_DIVISOR = 1.56;

// ================= DS18B20 =================
#define PINO_TEMP 27
OneWire oneWire(PINO_TEMP);
DallasTemperature sensors(&oneWire);

// ================= UART =================
#define RX2 16
#define TX2 17
String recebido = "";

// ================= TEMPO =================
unsigned long ultimoPrint     = 0;
const unsigned long intervaloPrint = 4000;

// ===== Dados recebidos do ESP1 =====
float rx_ch4   = 0;
float rx_nh3   = 0;
float rx_co2   = 0;
float rx_tvoc  = 0;
float rx_dist  = 0;

// ===== LCD páginas =====
int pagina = 0;
unsigned long ultimaTroca = 0;
const unsigned long intervaloTroca = 2500;

// =====================================================
// FUNÇÃO: extrai campo do pacote
// =====================================================
bool getCampo(String s, String chave, float &dest) {
  int i = s.indexOf(chave);
  if (i < 0) return false;
  i += chave.length();
  int f = s.indexOf(';', i);
  if (f < 0) f = s.length();
  dest = s.substring(i, f).toFloat();
  return true;
}

// =====================================================
// LCD
// =====================================================
void draw(float temperatura, float tds, float ntu, float ph) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 10, "Monitoramento Leite");

  if (pagina == 0) {
    u8g2.setCursor(0, 25); u8g2.printf("Temp: %.2f C", temperatura);
    u8g2.setCursor(0, 35); u8g2.printf("TDS : %.0f ppm", tds);
    u8g2.setCursor(0, 45); u8g2.printf("Turb: %.0f NTU", ntu);
    u8g2.setCursor(0, 55); u8g2.printf("pH  : %.2f", ph);
  } else {
    u8g2.drawStr(0, 20, "Valores ESP1");
    u8g2.setCursor(0, 30); u8g2.printf("CH4: %.0f%%", rx_ch4);
    u8g2.setCursor(0, 40); u8g2.printf("NH3: %.0f%%", rx_nh3);
    u8g2.setCursor(0, 50); u8g2.printf("CO2: %.0f", rx_co2);
    u8g2.setCursor(0, 60); u8g2.printf("Dist: %.0f", rx_dist);
  }

  u8g2.sendBuffer();

  if (millis() - ultimaTroca > intervaloTroca) {
    pagina = (pagina + 1) % 2;
    ultimaTroca = millis();
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
  while (Serial2.available()) Serial2.read();

  Wire.begin(21, 22);
  ads.begin();
  ads.setGain(GAIN_TWOTHIRDS);
  sensors.begin();
  u8g2.begin();

  Serial.println("ESP2 SLAVE pronto");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  // ===== TEMPERATURA =====
  sensors.requestTemperatures();
  float temperatura = sensors.getTempCByIndex(0);
  if (temperatura == DEVICE_DISCONNECTED_C) temperatura = 0;

  // ===== TDS =====
  int16_t adc_tds = ads.readADC_SingleEnded(1);
  float v_tds = (adc_tds * mvPorBit) / 1000.0 * FATOR_DIVISOR;
  float tds = 133.42 * pow(v_tds, 3) - 255.86 * pow(v_tds, 2) + 857.39 * v_tds;
  if (tds < 0) tds = 0;

  // ===== TURBIDEZ =====
  int16_t adc_ntu = ads.readADC_SingleEnded(2);
  float v_ntu = (adc_ntu * mvPorBit) / 1000.0 * FATOR_DIVISOR;
  float ntu;
  if (v_ntu < 2.5)       ntu = 3000;
  else if (v_ntu > 4.2)  ntu = 0;
  else                   ntu = -1120.4 * pow(v_ntu, 2) + 5742.3 * v_ntu - 4353.8;
  if (ntu < 0) ntu = 0;

  // ===== PH =====
  int16_t adc_ph = ads.readADC_SingleEnded(3);
  float v_ph = (adc_ph * mvPorBit) / 1000.0 * FATOR_DIVISOR;
  float ph = 7 + ((2.5 - v_ph) * 3.5);

  // ===== UART ESP1 ⇄ ESP2 =====
  if (Serial2.available()) {
    recebido = Serial2.readStringUntil('\n');
    recebido.trim();

    Serial.print("[ESP2] RX ESP1 -> ");
    Serial.println(recebido);

    // ===== Recebe valores do ESP1 =====
    if (recebido.indexOf("CH4:") >= 0) {
      getCampo(recebido, "CH4:",  rx_ch4);
      getCampo(recebido, "NH3:",  rx_nh3);
      getCampo(recebido, "CO2:",  rx_co2);
      getCampo(recebido, "TVOC:", rx_tvoc);
      getCampo(recebido, "DIST:", rx_dist);
    }
    // ===== ESP1 solicita dados do ESP2 =====
    else if (recebido == "REQ") {
      String pacoteESP2 =
        "TEMP:" + String(temperatura,2) + ";" +
        "TDS:"  + String(tds,0)         + ";" +
        "NTU:"  + String(ntu,0)         + ";" +
        "PH:"   + String(ph,2);

      Serial.print("[ESP2] TX ESP2 -> ");
      Serial.println(pacoteESP2);
      Serial2.println(pacoteESP2);
    }
  }

  // ===== SERIAL DEBUG =====
  if (millis() - ultimoPrint >= intervaloPrint) {
    ultimoPrint = millis();

    Serial.println("===== ESP2 =====");
    Serial.printf("Temp: %.2f C | TDS: %.0f | NTU: %.0f | pH: %.2f\n",
                  temperatura, tds, ntu, ph);
    Serial.println("===== ESP1 =====");
    Serial.printf("CH4: %.0f%% | NH3: %.0f%% | CO2: %.0f | TVOC: %.0f | DIST: %.1f\n\n",
                  rx_ch4, rx_nh3, rx_co2, rx_tvoc, rx_dist);
  }

  draw(temperatura, tds, ntu, ph);
}
