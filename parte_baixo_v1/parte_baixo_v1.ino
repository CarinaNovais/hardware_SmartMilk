//ESP 2 PARTE DE BAIXO
//funcionando 12/12/2025

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

// Divisor R1=5.6k / R2=10k
const float FATOR_DIVISOR = 1.56;

// Calibração TDS
float calibrationFactor = 1.0;

// DS18B20
#define PINO_TEMP 25
OneWire oneWire(PINO_TEMP);
DallasTemperature sensors(&oneWire);

//UART (RX2/TX2)
#define RX2 16
#define TX2 17

// Recepção do outro ESP
String recebido = "";

// Variáveis recebidas
float rx_ch4 = 0, rx_nh3 = 0, rx_co2 = 0, rx_tvoc = 0, rx_dist = 0;

// ====================================
// EXIBIR NO DISPLAY
// ====================================

// Controle da página atual
int pagina = 0; // 0 = locais, 1 = remotos
unsigned long ultimaTroca = 0;
const unsigned long intervaloTroca = 2000; // troca a cada 2 segundos

void draw(float temperatura, float densidade, float turbidez, float ph,
          float ch4, float nh3, float co2, float tvoc, float dist) {

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);

  // título
  u8g2.drawStr(0, 10, "Monitoramento Leite");

  if (pagina == 0) {
    // --- Página 1: dados locais ---
    u8g2.setCursor(0, 25);
    u8g2.printf("Temp: %.2f C", temperatura);

    u8g2.setCursor(0, 35);
    u8g2.printf("Dens: %.2f ppm", densidade);

    u8g2.setCursor(0, 45);
    u8g2.printf("Turb: %.1f NTU", turbidez);

    u8g2.setCursor(0, 55);
    u8g2.printf("pH: %.2f", ph);

  } else {
    // --- Página 2: dados remotos ---
    u8g2.drawStr(0, 25, "ESP1");

    u8g2.setCursor(0, 35);
    u8g2.printf("CH4: %.1f", ch4);

    u8g2.setCursor(0, 45);
    u8g2.printf("NH3: %.1f", nh3);

    u8g2.setCursor(0, 55);
    u8g2.printf("CO2: %.0f", co2);

    // Se quiser mostrar TVOC ou DIST, pode alternar em outra página
    // ou substituir algum item
  }

  u8g2.sendBuffer();

  // --- Alterna página automaticamente ---
  if (millis() - ultimaTroca > intervaloTroca) {
    pagina = (pagina + 1) % 2; // alterna entre 0 e 1
    ultimaTroca = millis();
  }
}


// ====================================
// SETUP
// ====================================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("Receptor iniciado...");

  Wire.begin(21, 22);

  if (!ads.begin()) {
    Serial.println("Erro ao iniciar ADS1115!");
    while (1);
  }

  ads.setGain(GAIN_TWOTHIRDS);

  sensors.begin();
  u8g2.begin();

  Serial.println("Iniciando leituras...");
}

// ====================================
// LOOP PRINCIPAL
// ====================================
void loop() {

  // -------------------------------------------------------
  // TEMPERATURA
  // -------------------------------------------------------
  sensors.requestTemperatures();
  float temperatura = sensors.getTempCByIndex(0);
  bool erroTemp = false;

  if (temperatura == DEVICE_DISCONNECTED_C) {
    erroTemp = true;
    temperatura = 25.0; 
  }

  // -------------------------------------------------------
  // TDS no A1
  // -------------------------------------------------------
  int16_t adc_tds = ads.readADC_SingleEnded(1);
  float v_out_tds = (adc_tds * mvPorBit) / 1000.0;
  float v_real_tds = v_out_tds * FATOR_DIVISOR;

  float tds = (133.42 * pow(v_real_tds, 3)
             - 255.86 * pow(v_real_tds, 2)
             + 857.39 * v_real_tds) * calibrationFactor;

  float coefTemp = 1.0 + 0.02 * (temperatura - 25.0);
  tds /= coefTemp;
  if (tds < 0) tds = 0;

  // -------------------------------------------------------
  // TURBIDEZ no A2
  // -------------------------------------------------------
  int16_t adc_ntu = ads.readADC_SingleEnded(2);
  float v_out_ntu = (adc_ntu * mvPorBit) / 1000.0;
  float v_real_ntu = v_out_ntu * FATOR_DIVISOR;

  float ntu;
  if (v_real_ntu < 2.5) ntu = 3000;
  else if (v_real_ntu > 4.2) ntu = 0;
  else ntu = -1120.4 * pow(v_real_ntu, 2) + 5742.3 * v_real_ntu - 4353.8;

  if (ntu < 0) ntu = 0;

  // -------------------------------------------------------
  // pH no A3
  // -------------------------------------------------------
  int16_t adc_ph = ads.readADC_SingleEnded(3);
  float v_out_ph = (adc_ph * mvPorBit) / 1000.0;
  float v_real_ph = v_out_ph * FATOR_DIVISOR;

  float ph = 7 + ((2.5 - v_real_ph) * 3.5);

  // -------------------------------------------------------
  // EXIBIR NO LCD
  // -------------------------------------------------------
  draw(temperatura, tds, ntu, ph, rx_ch4, rx_nh3, rx_co2, rx_tvoc, rx_dist);

  // -------------------------------------------------------
  // ENVIAR DADOS PARA O OUTRO ESP
  // -------------------------------------------------------
  String pacote = 
    "TEMP:"+ String(temperatura, 2) + "," +
    "TDS:" + String(tds, 2) + "," +
    "NTU:" + String(ntu, 2) + "," +
    "PH:" + String(ph, 2);

  Serial2.println(pacote);

  // -------------------------------------------------------
  // RECEBER DO OUTRO ESP
  // -------------------------------------------------------
  if (Serial2.available()) {

    recebido = Serial2.readStringUntil('\n');
    Serial.print("Recebido: ");
    Serial.println(recebido);

    rx_ch4  = getValor(recebido, "CH4:");
    rx_nh3  = getValor(recebido, "NH3:");
    rx_co2  = getValor(recebido, "CO2:");
    rx_tvoc = getValor(recebido, "TVOC:");
    rx_dist = getValor(recebido, "DIST:");

    Serial.print("CH4 -> ");  Serial.println(rx_ch4);
    Serial.print("NH3 -> ");  Serial.println(rx_nh3);
    Serial.print("CO2 -> ");  Serial.println(rx_co2);
    Serial.print("TVOC -> "); Serial.println(rx_tvoc);
    Serial.print("DIST -> "); Serial.println(rx_dist);
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
