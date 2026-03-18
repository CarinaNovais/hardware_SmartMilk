// ===============================================
// === SENSOR MQ-135 (AMÔNIA)
// ===============================================

// --- PINO ---
#define PIN_MQ135 35

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
void setup() {

  Serial.begin(115200);
  delay(2000);

  Serial.println("Teste Sensor MQ135 iniciado");

  analogReadResolution(12); // ESP32 -> 0 a 4095
}

void loop() {

  // leitura do sensor
  raw_mq135 = analogRead(PIN_MQ135);

  // classificação da amônia
  amonia = classifyMQ135(raw_mq135);

  // ===== SERIAL =====
  Serial.println("------ MQ135 ------");

  Serial.print("ADC bruto: ");
  Serial.println(raw_mq135);

  Serial.print("Amonia (% estimado): ");
  Serial.println(amonia);

  Serial.println("-------------------\n");

  delay(1000);
}