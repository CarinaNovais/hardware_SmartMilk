// ESP 2 - LCD + Sensores locais SLAVE
// Atualizado para protocolo UART com OK + %

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <U8g2lib.h>

// LCD ST7920 SPI via Software
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, 18, 4, 5, 26);

// ADS1115
Adafruit_ADS1115 ads;
const float mvPorBit = 0.1875;

// Divisor Resistivo
const float FATOR_DIVISOR = 1.56;

// DS18B20
#define PINO_TEMP 27
OneWire oneWire(PINO_TEMP);
DallasTemperature sensors(&oneWire);

// UART
#define RX2 16
#define TX2 17

String recebido = "";

// ===== Dados recebidos do ESP1 =====
int   rx_ch4_ok  = 1;
float rx_ch4_p   = 0;
int   rx_nh3_ok  = 1;
float rx_nh3_p   = 0;
float rx_co2     = 0;
float rx_tvoc    = 0;
float rx_dist    = 0;

// ===== LCD páginas =====
int pagina = 0;
unsigned long ultimaTroca = 0;
const unsigned long intervaloTroca = 2500;

// ==================================== desenhar lcd
void draw(float temperatura, float tds, float ntu, float ph) {

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);

  u8g2.drawStr(0, 10, "Monitoramento Leite");

  if (pagina == 0) {
    // ---- Dados locais ----
    u8g2.setCursor(0, 25);
    u8g2.printf("Temp: %.2f C", temperatura);

    u8g2.setCursor(0, 35);
    u8g2.printf("TDS : %.0f ppm", tds);

    u8g2.setCursor(0, 45);
    u8g2.printf("Turb: %.0f NTU", ntu);

    u8g2.setCursor(0, 55);
    u8g2.printf("pH  : %.2f", ph);

  } else {
// ---- Dados recebidos do ESP1 ----
    u8g2.drawStr(0, 25, "Qualidade do Ar");

    u8g2.setCursor(0, 35);
    u8g2.printf("CH4: %s %.0f%%", rx_ch4_ok ? "OK" : "ALERTA", rx_ch4_p);

    u8g2.setCursor(0, 45);
    u8g2.printf("NH3: %s %.0f%%", rx_nh3_ok ? "OK" : "ALERTA", rx_nh3_p);

    u8g2.setCursor(0, 55);
    u8g2.printf("CO2: %.0f ppm", rx_co2);
  }

  u8g2.sendBuffer();

  if (millis() - ultimaTroca > intervaloTroca) {
    pagina = (pagina + 1) % 2;
    ultimaTroca = millis();
  }
}



// ====================================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
  //limpando buffer
  while (Serial2.available()) Serial2.read();

  Wire.begin(21, 22);

  ads.begin();
  ads.setGain(GAIN_TWOTHIRDS);

  sensors.begin();
  u8g2.begin();

  Serial.println("ESP2 SLAVE pronto");
}

// ====================================

void printSerial(float temp, float tds, float ntu, float ph) {
  Serial.println("===== ESP2 =====");
  Serial.printf("Temp: %.2f C\n", temp);
  Serial.printf("TDS : %.0f ppm\n", tds);
  Serial.printf("Turb: %.0f NTU\n", ntu);
  Serial.printf("pH  : %.2f\n\n", ph);

  Serial.printf("CH4: %s (%.0f%%)\n", rx_ch4_ok ? "OK" : "ALERTA", rx_ch4_p);
  Serial.printf("NH3: %s (%.0f%%)\n", rx_nh3_ok ? "OK" : "ALERTA", rx_nh3_p);
  Serial.printf("CO2: %.0f ppm\n", rx_co2);
  Serial.printf("TVOC: %.0f ppb\n", rx_tvoc);
  Serial.printf("DIST: %.1f cm\n", rx_dist);
  Serial.println("================\n");
}

void loop() {

  // ===== Temperatura ================================================
  sensors.requestTemperatures();
  float temperatura = sensors.getTempCByIndex(0);
  if (temperatura == DEVICE_DISCONNECTED_C)
    temperatura = 00.09;

  // ===== TDS =======================================================
  int16_t adc_tds = ads.readADC_SingleEnded(1);
  float v_tds = (adc_tds * mvPorBit) / 1000.0 * FATOR_DIVISOR;

  float tds = (133.42 * pow(v_tds, 3)
             - 255.86 * pow(v_tds, 2)
             + 857.39 * v_tds);

  if (tds < 0) tds = 0;

  // ===== Turbidez ==================================================
  int16_t adc_ntu = ads.readADC_SingleEnded(2);
  float v_ntu = (adc_ntu * mvPorBit) / 1000.0 * FATOR_DIVISOR;

  float ntu;
  if (v_ntu < 2.5) ntu = 3000;
  else if (v_ntu > 4.2) ntu = 0;
  else ntu = -1120.4 * pow(v_ntu, 2) + 5742.3 * v_ntu - 4353.8;

  if (ntu < 0) ntu = 0;

  // ===== pH ===========================================================
  int16_t adc_ph = ads.readADC_SingleEnded(3);
  float v_ph = (adc_ph * mvPorBit) / 1000.0 * FATOR_DIVISOR;
  float ph = 7 + ((2.5 - v_ph) * 3.5);

  //AGAURADADO COMANDO DO ESP1============================================
 if (Serial2.available()) {
  recebido = Serial2.readStringUntil('\n');
  recebido.trim();

  // 1) Pedido de dados
  if (recebido == "REQ") {
    String pacote =
      "Temp:" + String(temperatura,2) + "," +
      "TDS:"  + String(tds,1) + "," +
      "NTU:"  + String(ntu,1) + "," +
      "PH:"   + String(ph,2);
      
      Serial2.println(pacote);
    } else { //recebendo 
   // recebido = Serial2.readStringUntil('\n');
      rx_ch4_ok = getValor(recebido, "CH4_OK:");
      rx_ch4_p  = getValor(recebido, "CH4_P:");
      rx_nh3_ok = getValor(recebido, "NH3_OK:");
      rx_nh3_p  = getValor(recebido, "NH3_P:");
      rx_co2    = getValor(recebido, "CO2:");
      rx_tvoc   = getValor(recebido, "TVOC:");
      rx_dist   = getValor(recebido, "DIST:");
    }
  }

  // ===== LCD =====
  draw(temperatura, tds, ntu, ph);
  printSerial(temperatura, tds, ntu, ph);
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
