// ========================================================
// === MÓDULO 3: SENSOR DE PH
// ========================================================

// --- PINOS E VARIÁVEIS ---
const int pinPh = 32;
float PhFinal = 0;
String statusPh = "N/A";

// --- FUNÇÃO DE LEITURA ---
void lerPh() {
  float leituraAnalogPh = analogRead(pinPh);
  float voltagemPH = leituraAnalogPh * (VREF / ADC_MAX);
  
  // Fórmula de calibração linear (pH = 7.0 + ((V_Meio - V_Lido) / Slope))
  PhFinal = 7.0 + ((1.65f - voltagemPH) / 0.18f); 
  PhFinal = ArredondarPara(PhFinal, 2);

  // Classificação do Status
  statusPh = (PhFinal >= 6.4 && PhFinal <= 7.0) ? "Adequado" : "Inadequado";
}