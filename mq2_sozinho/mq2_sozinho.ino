// ===============================================
// === SENSOR MQ-2 (METANO)
// ===============================================

// --- PINO ---
#define PIN_MQ2    34

// --- VARIÁVEIS ---
int raw_mq2   = 0; 
float metano = 0.0f;

// --- FUNÇÃO DE CLASSIFICAÇÃO ---
float classifyMQ2(int raw) {
  if (raw <= 150) return 10.0f;
  else if (raw <= 400) return 30.0f;
  else if (raw <= 700) return 50.0f;
  else return 100.0f;
}