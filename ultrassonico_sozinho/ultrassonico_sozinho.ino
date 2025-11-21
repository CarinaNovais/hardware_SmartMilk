// ===== CONFIGURAÇÕES E VARIÁVEIS DO ULTRASSÔNICO =====
#define PIN_TRIG   14
#define PIN_ECHO   4

const unsigned long TIMEOUT_US = 30000UL; 
const float SOUND_CM_PER_US = 0.0343f; 
const int NUM_SAMPLES = 5;                

// Buffer para Média Móvel
float samples[NUM_SAMPLES];
int sampleIndex = 0;
bool samplesFilled = false;

// Variáveis de Saída
float distancia = 0.0f;
float altura_agua = 0.0f;
float litros = 0.0f;

// Metadados do Tanque
float raio_cm = 9.0f;          // cm
float altura_tanque_cm = 18.5f;  // cm


// ===== FUNÇÕES DO SENSOR ULTRASSONICO =====

float measureDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); 
  delayMicroseconds(10); 
  digitalWrite(PIN_TRIG, LOW); 

  unsigned long duration = pulseIn(PIN_ECHO, HIGH, TIMEOUT_US); 

  if (duration == 0) {
    return -1.0f; // timeout / erro
  }
  return (duration * SOUND_CM_PER_US) / 2.0f;
}

float getSmoothedDistance() {
  float d = measureDistanceCm();
  if (d >= 0.0f) {
    samples[sampleIndex] = d;
    sampleIndex++;
    if (sampleIndex >= NUM_SAMPLES) { sampleIndex = 0; samplesFilled = true; }
  } else {
    if (!samplesFilled && sampleIndex == 0) return -1.0f;
  }

  int count = samplesFilled ? NUM_SAMPLES : sampleIndex;
  if (count == 0) return -1.0f;

  float sum = 0.0f;
  int valid = 0;
  for (int i = 0; i < count; ++i) {
    if (samples[i] > 0) {
      sum += samples[i];
      valid++;
    }
  }
  if (valid == 0) return -1.0f;
  return sum / (float)valid;
}

// Lógica de Medição e Cálculo de Volume (para ser chamada no loop())
void updateVolume() {
  float leituraNova = getSmoothedDistance();
  
  if (leituraNova >= 0.0f) {
      distancia = leituraNova;
  } else {
      Serial.println("Erro leitura ultrassônico (timeout)");
  }
  
  altura_agua = clampFloorZero(altura_tanque_cm - distancia);
  float volume_cm3 = 3.14159f * raio_cm * raio_cm * altura_agua; // Volume Cilindro (cm³)
  litros = volume_cm3 / 1000.0f;  // Conversão para Litros
}