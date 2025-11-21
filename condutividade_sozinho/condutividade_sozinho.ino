// ========================================================
// === MÓDULO 4: SENSOR TDS / CONDUTIVIDADE
// ========================================================

// --- PINOS E VARIÁVEIS ---
const int pinTDS = 34;
float tdsFinal = 0;
float calibrationFactorTDS = 1.00f; 
String statusCondutividade = "N/A";

// --- FUNÇÃO DE LEITURA E PROCESSAMENTO ---
void lerTDS(float temperaturaAtual) {
  int readings[NUM_MEDIAN_TDS];
  
  for (int i = 0; i < NUM_MEDIAN_TDS; ++i) {
    readings[i] = analogRead(pinTDS);
    delay(20); 
  }
  
  int med = medianFilter(readings, NUM_MEDIAN_TDS); 
  float voltage = ((float)med / (float)ADC_MAX) * VREF;
  
  // Converte tensão em TDS (ppm) - Polinômio
  float tdsValue = 133.42f * powf(voltage, 3.0f) - 255.86f * powf(voltage, 2.0f) + 857.39f * voltage;
  tdsValue *= calibrationFactorTDS;
  
  // Compensação de temperatura (k=0.02f)
  float k = 0.02f;
  float tdsCompensado = tdsValue / (1.0f + k * (temperaturaAtual - 25.0f));
  
  tdsFinal = (tdsCompensado < 0) ? 0 : tdsCompensado;
  
  // Classificação do Status
  if (tdsFinal < 300) { 
    statusCondutividade = "Baixo"; 
  } else if (tdsFinal < 1000) {
    statusCondutividade = "Medio";
  } else {
    statusCondutividade = "Alto";
  }
}