// ========================================================
// === SENSOR DE TURBIDEZ
// ========================================================

// --- CONFIGURAÇÕES DO ESP32 ---
const int pinTurbidez = 33;

const int NUM_SAMPLES_TURB = 10; // quantidade de leituras médias
const float VREF = 3.3;          // tensão usada pelo ADC
const int ADC_MAX = 4095;        // resolução do ADC do ESP32 (12 bits)

float ntuFinal = 0;
String statusTurbidez = "N/A";

// --------------------------------------------------------
// Função auxiliar para arredondar valores
// --------------------------------------------------------
float ArredondarPara(float valor, int casas)
{
  float mult = pow(10, casas);
  return round(valor * mult) / mult;
}

// --------------------------------------------------------
// Função de leitura da turbidez
// --------------------------------------------------------
void lerTurbidez()
{
  float voltagem = 0;

  for (int i = 0; i < NUM_SAMPLES_TURB; i++)
  {
    float leitura = analogRead(pinTurbidez);
    float v = (leitura / (float)ADC_MAX) * VREF;
    voltagem += v;
    delay(5);
  }

  voltagem = voltagem / NUM_SAMPLES_TURB;
  voltagem = ArredondarPara(voltagem, 2);

  float ntu = 0;

  // Curva aproximada do sensor
  if (voltagem < 2.5)
  {
    ntu = 3000;
  }
  else if (voltagem > 4.2)
  {
    ntu = 0;
  }
  else
  {
    ntu = -1120.4 * pow(voltagem, 2) + 5742.3 * voltagem - 4353.8;
  }

  ntuFinal = (ntu < 0) ? 0 : ntu;

  // Classificação da turbidez
  if (ntuFinal <= 5)
  {
    statusTurbidez = "Limpa";
  }
  else if (ntuFinal <= 50)
  {
    statusTurbidez = "Media";
  }
  else
  {
    statusTurbidez = "Suja";
  }
}

// ========================================================
// SETUP
// ========================================================
void setup()
{
  Serial.begin(115200);
  delay(500);

  pinMode(pinTurbidez, INPUT);

  Serial.println("Sensor de Turbidez iniciado!");
}

// ========================================================
// LOOP
// ========================================================
void loop()
{
  lerTurbidez();

  Serial.print("Turbidez: ");
  Serial.print(ntuFinal);
  Serial.print(" NTU | Status: ");
  Serial.println(statusTurbidez);

  delay(500);
}
