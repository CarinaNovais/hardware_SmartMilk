// ========================================================
// === MÓDULO 3: SENSOR DE pH usando ADS1115 + ESP32
// ========================================================

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

// ----- VARIÁVEIS -----
float phFinal = 0;
String statusPh = "N/A";

// Coeficientes da calibração (EXEMPLO — ajuste quando calibrar)
// ph = 7 + ((V_meio - V) / slope)
float Vneutral = 2.50;   // tensão quando o pH = 7 (ajuste!)
float slope    = 0.18;   // sensibilidade em V/pH (ajuste!)


// --------------------------------------------------------
// Função auxiliar para arredondar valores
// --------------------------------------------------------
float ArredondarPara(float valor, int casas) {
  float mult = pow(10, casas);
  return round(valor * mult) / mult;
}


// --------------------------------------------------------
// Leitura do pH via ADS1115
// --------------------------------------------------------
void lerPh() {

  // Lê canal A0 do ADS1115
  int16_t leitura = ads.readADC_SingleEnded(0);

  // Converte para tensão (GAIN = 1 → 4.096V máximo)
  float volts = leitura * 0.000125;   // 0.000125 = 4.096 / 32767

  // ---- Cálculo do pH ----
  phFinal = 7.0 + ((Vneutral - volts) / slope);
  phFinal = ArredondarPara(phFinal, 2);

  // ---- Classificação ----
  if (phFinal >= 6.4 && phFinal <= 7.0) {
    statusPh = "Adequado";
  } else {
    statusPh = "Inadequado";
  }
}


// ========================================================
// SETUP
// ========================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(21, 22);  // SDA, SCL do ESP32

  if (!ads.begin()) {
    Serial.println("Erro ao iniciar ADS1115!");
    while (1);
  }

  ads.setGain(GAIN_ONE); // 1x = até 4.096V

  Serial.println("Sensor de pH iniciado!");
}


// ========================================================
// LOOP
// ========================================================
void loop() {
  lerPh();

  Serial.print("pH: ");
  Serial.print(phFinal);
  Serial.print(" | Status: ");
  Serial.println(statusPh);

  delay(500);
}
