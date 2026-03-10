// ===============================================
// === SENSOR MQ-135 (AMÔNIA)
// ===============================================

// --- PINO ---
#define PIN_MQ135  35

// --- VARIÁVEIS ---
int raw_mq135 = 0; 
float amonia = 0.0f;

// --- FUNÇÃO DE CLASSIFICAÇÃO ---
float classifyMQ135(int raw) {
  if (raw <= 150) return 10.0f;
  else if (raw <= 400) return 30.0f;
  else if (raw <= 700) return 50.0f;
  else return 100.0f;
}