

// ===== CONFIGURAÇÕES E VARIÁVEIS DO ULTRASSÔNICO =====
#define PIN_TRIG 14
#define PIN_ECHO 4

const unsigned long TIMEOUT_US = 30000UL; // 30ms
const float SOUND_CM_PER_US = 0.0343f;    // Velocidade do som (cm/us)
const int NUM_SAMPLES = 5;                // Amostras para média móvel

// Buffer da média móvel
float samples[NUM_SAMPLES];
int sampleIndex = 0;
bool samplesFilled = false;

// Variáveis calculadas
float distancia = 0.0f;
float altura_agua = 0.0f;
float litros = 0.0f;

// Medidas do tanque
float raio_cm = 9.0f;           // cm
float altura_tanque_cm = 18.5f; // cm

// =======================================================
//  Função clamp - impede valores negativos
// =======================================================
float clampFloorZero(float x)
{
  return (x < 0.0f) ? 0.0f : x;
}

// =======================================================
//  Mede distância com o JSN-SR04T
// =======================================================
float measureDistanceCm()
{
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long duration = pulseIn(PIN_ECHO, HIGH, TIMEOUT_US);

  if (duration == 0)
  {
    return -1.0f; // timeout
  }

  return (duration * SOUND_CM_PER_US) / 2.0f;
}

// =======================================================
//  Faz média móvel da distância
// =======================================================
float getSmoothedDistance()
{
  float d = measureDistanceCm();

  if (d >= 0.0f)
  {
    samples[sampleIndex] = d;
    sampleIndex++;

    if (sampleIndex >= NUM_SAMPLES)
    {
      sampleIndex = 0;
      samplesFilled = true;
    }
  }
  else
  {
    if (!samplesFilled && sampleIndex == 0)
      return -1.0f;
  }

  int count = samplesFilled ? NUM_SAMPLES : sampleIndex;
  if (count == 0)
    return -1.0f;

  float sum = 0.0f;
  int valid = 0;

  for (int i = 0; i < count; ++i)
  {
    if (samples[i] > 0)
    {
      sum += samples[i];
      valid++;
    }
  }

  if (valid == 0)
    return -1.0f;

  return sum / float(valid);
}

// =======================================================
//  Atualiza distância, altura e litros (função principal)
// =======================================================
void updateVolume()
{
  float leituraNova = getSmoothedDistance();

  if (leituraNova >= 0.0f)
  {
    distancia = leituraNova;
  }
  else
  {
    Serial.println("Erro: timeout ultrassônico");
  }

  altura_agua = clampFloorZero(altura_tanque_cm - distancia);

  float volume_cm3 =
      3.14159f * raio_cm * raio_cm * altura_agua;

  litros = volume_cm3 / 1000.0f;
}

// =======================================================
//  SETUP
// =======================================================
void setup()
{
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW); 

  Serial.println("Sensor ultrassonico iniciado!");
}

// =======================================================
//  LOOP
// =======================================================
void loop()
{
  updateVolume();

  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.print(" cm | Altura: ");
  Serial.print(altura_agua);
  Serial.print(" cm | Litros: ");
  Serial.println(litros);

  delay(500);
}
