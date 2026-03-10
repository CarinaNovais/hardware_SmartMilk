#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h> // temperatura
#include <DallasTemperature.h> // temperatura
#include <U8g2lib.h> // lcd

// lcd st8920 SPI via Software
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, 18, 4, 5, 26);

//ads1115
Adafruit_ADS1115 ads;

// Conversão do ADS1115 (GAIN_TWOTHIRDS)
const float mvPorBit = 0.1875;

// Fator do divisor (R1=5.6k, R2=10k → 1/0.641 = 1.56)
const float FATOR_DIVISOR = 1.56;

// Calibração fina para TDS
float calibrationFactor = 1.0;

// ---------------- DS18B20 TEMPERATURA  ----------------
#define PINO_TEMP 25        // DATA do sensor
OneWire oneWire(PINO_TEMP);
DallasTemperature sensors(&oneWire);


// ====================================
// EXIBIR NO DISPLAY
// ====================================
void draw(float temperatura, float densidade, float turbidez, float ph) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);

  u8g2.drawStr(0, 10, "Monitoramento Agua");

  u8g2.setCursor(0, 25);
  u8g2.printf("Temp: %.2f C", temperatura);

  u8g2.setCursor(0, 35);
  u8g2.printf("Dens: %.2f ppm", densidade);

  u8g2.setCursor(0, 45);
  u8g2.printf("Turb: %.1f NTU", turbidez);

  u8g2.setCursor(0, 55);
  u8g2.printf("pH: %.2f", ph);

  u8g2.sendBuffer();
}
//==========================
//setup
// =========================

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA=21, SCL=22 no ESP32

  if (!ads.begin()) {
    Serial.println("Erro ao iniciar ADS1115!");
    while (1);
  }

  ads.setGain(GAIN_TWOTHIRDS);  // ±6.144V
  sensors.begin(); //inicia sensor temperatura 
  u8g2.begin(); 
  Serial.println("Iniciando leituras: TDS | Turbidez | Temperatura (DS18B20)");

}

//==================
// loop
//===========

void loop() {
   // ============================
  //   TEMPERATURA DS18B20
  // ============================
  sensors.requestTemperatures();
  float temperatura = sensors.getTempCByIndex(0);  
  if (temperatura == DEVICE_DISCONNECTED_C) {
    Serial.println("Erro no sensor DS18B20!");
    temperatura = 25.0; // usa valor padrão
  }


  // ============================
  // Leitura de TDS (canal A1)
  // ============================
  int16_t adc_tds = ads.readADC_SingleEnded(1);
  float v_out_tds = (adc_tds * mvPorBit) / 1000.0;
  float v_real_tds = v_out_tds * FATOR_DIVISOR;

  float tds = (133.42 * pow(v_real_tds, 3)
             - 255.86 * pow(v_real_tds, 2)
             + 857.39 * v_real_tds) * calibrationFactor;

  // Compensação de temperatura
  float coefTemp = 1.0 + 0.02 * (temperatura - 25.0);
  tds = tds / coefTemp;
  if (tds < 0) tds = 0;

  // ============================
  // Leitura de Turbidez (canal A2)
  // ============================
  int16_t adc_ntu = ads.readADC_SingleEnded(2);
  float v_out_ntu = (adc_ntu * mvPorBit) / 1000.0;
  float v_real_ntu = v_out_ntu * FATOR_DIVISOR;

  float ntu = 0;
  if (v_real_ntu < 2.5) {
    ntu = 3000;  // água muito suja
  } else if (v_real_ntu > 4.2) {
    ntu = 0;     // água limpa
  } else {
    ntu = -1120.4 * pow(v_real_ntu, 2)
        + 5742.3 * v_real_ntu
        - 4353.8;
  }
  if (ntu < 0) ntu = 0;

  //===============
  //ph falta fazer
  //=================

  //============
  //exibir no lcd
  //=============

  draw(temperatura, tds, ntu, ph);
  

  // ============================
  //   Exibir resultados SERIAL
  // ============================
  Serial.println("=======================================");
  Serial.print("Temperatura DS18B20: ");
  Serial.print(temperatura, 2);
  Serial.println(" °C");

  Serial.print("ADC A1 (TDS): "); Serial.print(adc_tds);
  Serial.print(" | Vout: "); Serial.print(v_out_tds, 4);
  Serial.print(" V | Vreal: "); Serial.print(v_real_tds, 4);
  Serial.print(" V | TDS: "); Serial.print(tds, 2);
  Serial.println(" ppm");

  Serial.print("ADC A2 (Turbidez): "); Serial.print(adc_ntu);
  Serial.print(" | Vout: "); Serial.print(v_out_ntu, 4);
  Serial.print(" V | Vreal: "); Serial.print(v_real_ntu, 4);
  Serial.print(" V | NTU: "); Serial.println(ntu, 1);

  delay(800);
}