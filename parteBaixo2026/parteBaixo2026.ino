//////////USAR ESSE

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>

// ================= ADS1115 =================
Adafruit_ADS1115 ads;
const float MV_POR_BIT = 0.1875;
const float FATOR_DIVISOR = 3.0;

// Canais ADS
#define CH_TDS 1  // A1
#define CH_TURB 2 // A2
#define CH_PH 3   // A3

const int AMOSTRAS = 50;

// ================= TDS / CONDUTIVIDADE =================
const float FATOR_CAL_TDS = 1.08; // calibrado
const float V_MIN_VALIDO = 0.40;  // ignora fora da água
float temperaturaC = 25.0;        // referência

// ================= TURBIDEZ =================
const float V_CLEAR = 3.74;
const float V_TURVA = 0.268;
const float NTU_TURVA = 1000.0;

// ================= pH =================
float V_PH7 = 2.50; // AJUSTAR quando usar solução pH 7
float PH_GAIN = 3.5;

// ================= DS18B20 =================
#define PINO_TEMP 27
OneWire oneWire(PINO_TEMP);
DallasTemperature sensors(&oneWire);

// ================= LCD =================
U8G2_ST7920_128X64_F_SW_SPI u8g2(
    U8G2_R0,
    18, // CLK
    4,  // MOSI
    5,  // CS
    26  // RST
);

// ===== LCD páginas =====
int pagina = 0;
unsigned long ultimaTroca = 0;
const unsigned long intervaloTroca = 2500;

// ================= UART =================
#define RX2 16
#define TX2 17
String recebido = "";

// ===== Dados recebidos do ESP1 =====
float rx_metano = 0;
float rx_amonia = 0;
float rx_co2 = 0;
float rx_nivel = 0;

// =====================================================
// FUNÇÃO: extrai campo do pacote
// =====================================================
bool getCampo(String s, String chave, float &dest)
{
  int i = s.indexOf(chave);
  if (i < 0)
    return false;
  i += chave.length();
  int f = s.indexOf(';', i);
  if (f < 0)
    f = s.length();
  dest = s.substring(i, f).toFloat();
  return true;
}

// =====================================================
// LCD
// =====================================================
void draw(float temperatura, float tds_cond, float turbidez, float ph)
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 10, "Monitoramento Leite");

  if (pagina == 0)
  {
    u8g2.setCursor(0, 25);
    u8g2.printf("Temp: %.2f C", temperatura);
    u8g2.setCursor(0, 35);
    u8g2.printf("tds_cond : %.0f ppm", tds_cond);
    u8g2.setCursor(0, 45);
    u8g2.printf("Turb: %.0f turbidez", turbidez);
    u8g2.setCursor(0, 55);
    u8g2.printf("pH  : %.2f", ph);
  }
  else
  {
    u8g2.drawStr(0, 20, "Valores ESP1");
    u8g2.setCursor(0, 30);
    u8g2.printf("metano: %.0f%%", rx_metano);
    u8g2.setCursor(0, 40);
    u8g2.printf("amonia: %.0f%%", rx_amonia);
    u8g2.setCursor(0, 50);
    u8g2.printf("co2: %.0f", rx_co2);
    u8g2.setCursor(0, 60);
    u8g2.printf("nivel: %.1f L", rx_nivel);
  }

  u8g2.sendBuffer();

  if (millis() - ultimaTroca > intervaloTroca)
  {
    pagina = (pagina + 1) % 2;
    ultimaTroca = millis();
  }
}

// =====================================================
float lerVoltsReaisADS(int canal)
{
  long soma = 0;
  for (int i = 0; i < AMOSTRAS; i++)
  {
    soma += ads.readADC_SingleEnded(canal);
    delay(2);
  }
  float adc_medio = soma / (float)AMOSTRAS;
  float v_ads = (adc_medio * MV_POR_BIT) / 1000.0;
  return v_ads * FATOR_DIVISOR;
}

// ================= TDS =================
float calcularTDS(float v)
{
  if (v < V_MIN_VALIDO)
    return 0;
  float coef = 1.0 + 0.02 * (temperaturaC - 25.0);
  float v_comp = v / coef;
  float tds = 133.42 * pow(v_comp, 3) - 255.86 * pow(v_comp, 2) + 857.39 * v_comp;
  tds *= FATOR_CAL_TDS;
  if (tds < 0)
    tds = 0;
  if (tds > 1000)
    tds = 1000;
  return tds;
}

// ================= TURBIDEZ =================
float calcularTurbidez(float v)
{
  if (v >= V_CLEAR)
    return 0;
  if (v <= V_TURVA)
    return NTU_TURVA;
  return (V_CLEAR - v) * (NTU_TURVA / (V_CLEAR - V_TURVA));
}

// ================= pH =================
float calcularPH(float v)
{
  float ph = 7.0 + ((V_PH7 - v) * PH_GAIN);
  if (ph < 0)
    ph = 0;
  if (ph > 14)
    ph = 14;
  return ph;
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
  while (Serial2.available())
    Serial2.read();

  Wire.begin(21, 22);

  if (!ads.begin())
  {
    Serial.println("ERRO: ADS1115 nao encontrado!");
    while (1)
      delay(100);
  }
  ads.setGain(GAIN_TWOTHIRDS);

  sensors.begin();

  u8g2.begin();

  Serial.println("Sistema de Sensores Iniciado");
}

// ================= LOOP =================
void loop()
{
  // Temperatura
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  if (temp != DEVICE_DISCONNECTED_C)
  {
    temperaturaC = temp;
  }

  // Leituras ADS
  float v_tds = lerVoltsReaisADS(CH_TDS);
  float v_turb = lerVoltsReaisADS(CH_TURB);
  float v_ph = lerVoltsReaisADS(CH_PH);

  // Conversões
  float tds = calcularTDS(v_tds);
  float turb = calcularTurbidez(v_turb);
  float ph = calcularPH(v_ph);

  // ===== UART ESP1 ⇄ ESP2 =====
  while (Serial2.available())
  {
    recebido = Serial2.readStringUntil('\n');
    recebido.trim();

    if (recebido.length() == 0)
      continue;

    Serial.print("[ESP2] RX ESP1 -> ");
    Serial.println(recebido);

    if (recebido.indexOf("metano:") >= 0)
    {
      getCampo(recebido, "metano:", rx_metano);
      getCampo(recebido, "amonia:", rx_amonia);
      getCampo(recebido, "co2:", rx_co2);
      getCampo(recebido, "nivel:", rx_nivel);
    }
    else if (recebido == "REQ")
    {
      String pacoteESP2 =
          "temp:" + String(temperaturaC, 2) + ";" +
          "tds_cond:" + String(tds, 0) + ";" +
          "turbidez:" + String(turb, 0) + ";" +
          "ph:" + String(ph, 2);

      Serial.print("[ESP2] TX ESP2 -> ");
      Serial.println(pacoteESP2);
      Serial2.println(pacoteESP2);
    }
  }

  // ===== PRINT SIMPLES =====
  Serial.print("Temp: ");
  Serial.print(temperaturaC, 2);
  Serial.print(" C | TDS: ");
  Serial.print(tds, 0);
  Serial.print(" ppm | Turb: ");
  Serial.print(turb, 0);
  Serial.print(" NTU | pH: ");
  Serial.println(ph, 2);

  draw(temperaturaC, tds, turb, ph);

  delay(1000);
}