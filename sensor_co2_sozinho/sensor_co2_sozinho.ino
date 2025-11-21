// ===============================================
// === SENSOR CO2 (ANALÓGICO)
// ===============================================

// --- PINO ---
#define PIN_CO2    32

// --- VARIÁVEIS ---
int raw_co2   = 0;
int co2_adc = 0; // Cópia da leitura bruta, para compatibilidade com JSON/LoRa
float co2_class = 0.0f;

// --- FUNÇÃO DE CLASSIFICAÇÃO ---
float classifyCO2(int raw) {
  // Mapeamento 0..4095 -> 0..100
  float pct = (raw / 4095.0f) * 100.0f;
  return pct;
}