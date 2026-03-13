#define MQ2_PIN 34
#define MQ135_PIN 35

// Divisor de tensão 20k/10k
const float R1 = 20000.0;     // 20k ligado ao 3.3V
const float R2 = 10000.0;     // 10k ligado ao GND
const float ADC_MAX = 4095.0; // ESP32 ADC 12 bits
const float VCC = 3.3;        // tensão do ESP32

// Resistência do sensor em ar limpo (R0), será calibrada automaticamente
float R0_MQ2 = 0;
float R0_MQ135 = 0;

// Tempo de calibração em milissegundos
const int calibTime = 5000;

void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("Calibrando sensores MQ-2 (CH4) e MQ-135 (NH3) em ar limpo...");

  // Calibração automática em ar limpo
  unsigned long start = millis();
  float sum_MQ2 = 0;
  float sum_MQ135 = 0;
  int count = 0;

  while (millis() - start < calibTime)
  {
    float Vin_MQ2 = readSensor(MQ2_PIN);
    float Vin_MQ135 = readSensor(MQ135_PIN);
    float Rs_MQ2 = getRs(Vin_MQ2);
    float Rs_MQ135 = getRs(Vin_MQ135);

    if (Rs_MQ2 > 0)
      sum_MQ2 += Rs_MQ2;
    if (Rs_MQ135 > 0)
      sum_MQ135 += Rs_MQ135;
    count++;
    delay(200);
  }

  R0_MQ2 = sum_MQ2 / count;
  R0_MQ135 = sum_MQ135 / count;

  Serial.print("Calibração completa! R0_MQ2 = ");
  Serial.print(R0_MQ2, 2);
  Serial.print(" Ω, R0_MQ135 = ");
  Serial.println(R0_MQ135, 2);
  Serial.println("Iniciando leitura de gases...");
}

void loop()
{
  float Vin_MQ2 = readSensor(MQ2_PIN);
  float Vin_MQ135 = readSensor(MQ135_PIN);

  float Rs_MQ2 = getRs(Vin_MQ2);
  float Rs_MQ135 = getRs(Vin_MQ135);

  // Proteção contra NaN
  float perc_CH4 = 0;
  if (R0_MQ2 > 0 && Rs_MQ2 > 0)
    perc_CH4 = getGasPercentage(Rs_MQ2, R0_MQ2);

  float perc_NH3 = 0;
  if (R0_MQ135 > 0 && Rs_MQ135 > 0)
    perc_NH3 = getGasPercentage(Rs_MQ135, R0_MQ135);

  Serial.print("Metano (CH4): ");
  Serial.print(perc_CH4, 1);
  Serial.print(" % | Amônia (NH3): ");
  Serial.print(perc_NH3, 1);
  Serial.println(" %");

  // Alertas simples
  if (perc_CH4 > 10)
    Serial.println("Alerta CH4!");
  if (perc_NH3 > 5)
    Serial.println("Alerta NH3!");

  delay(1000);
}

// ====================== Funções ======================

// Lê tensão real do sensor, corrigindo pelo divisor
float readSensor(int pin)
{
  int raw = analogRead(pin);
  float Vout = (raw / ADC_MAX) * VCC;
  float Vin = Vout * (R1 + R2) / R2;
  return Vin;
}

// Calcula resistência do sensor
float getRs(float Vin)
{
  if (Vin <= 0)
    return 0;
  return ((VCC * R2 / Vin) - R2);
}

// Converte Rs/R0 em porcentagem aproximada de gás
float getGasPercentage(float Rs, float R0)
{
  float ratio = Rs / R0;
  float A = 100.0;
  float B = -1.2;
  float perc = A * pow(ratio, B);
  if (perc < 0)
    perc = 0;
  if (perc > 100)
    perc = 100;
  return perc;
}
