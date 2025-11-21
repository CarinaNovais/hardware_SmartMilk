// ========================================================
// === MÓDULO 5: SENSOR DE TURBIDEZ
// ========================================================

// --- PINOS E VARIÁVEIS ---
const int pinTurbidez = 33;
float ntuFinal = 0;
String statusTurbidez = "N/A";

// --- FUNÇÃO DE LEITURA ---
void lerTurbidez() {
  float voltagem = 0;
  for (int i = 0; i < NUM_SAMPLES_TURB; i++) {
    voltagem += (analogRead(pinTurbidez) / (float)ADC_MAX) * VREF;
  }
  voltagem = voltagem / (float)NUM_SAMPLES_TURB;
  voltagem = ArredondarPara(voltagem, 2);

  float ntu = 0;
  // Polinômio (fórmula do sensor)
  if (voltagem < 2.5) {
    ntu = 3000;
  } else if (voltagem > 4.2) {
    ntu = 0;
  } else {
    ntu = -1120.4 * pow(voltagem, 2) + 5742.3 * voltagem - 4353.8;
  }
  ntuFinal = (ntu < 0) ? 0 : ntu;

  // Classificação do Status
  if (ntuFinal <= 5) statusTurbidez = "Limpa";
  else if (ntuFinal <= 50) statusTurbidez = "Media";
  else statusTurbidez = "Suja";
}