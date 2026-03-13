/*
 * SENSOR TDS / CONDUTIVIDADE NO ESP32
 */
// Resistor R1 (10k)
// Terminal 1 → AOUT do sensor TDS
// Terminal 2 → Nó VOUT (onde liga R2 e o ESP32)

// Resistor R2 (10k)
// Terminal 1 → Nó VOUT (junto com R1 e o fio do ESP32)
// Terminal 2 → GND

// Conexão no ESP32
// GPIO 34 ou 33 → VOUT (nó do divisor)
// GND do ESP32 → GND do divisor e do sensor

// Conexão da alimentação
// VCC do sensor TDS → 5V (VIN ou 5V da USB)
// GND do sensor TDS → GND do ESP32

// ========================================================
// === CONFIGURAÇÕES DO ESP32 E VARIÁVEIS ===
// ========================================================

const int pinTDS = 34;

// Resolução ADC ESP32
const float VREF = 3.3;
const int ADC_MAX = 4095;

// Leituras para filtro mediana
const int NUM_MEDIAN_TDS = 15;

float calibrationFactorTDS = 1.00f;
float tdsFinal = 0;
String statusCondutividade = "N/A";

// ========================================================
// === FUNÇÃO FILTRO DE MEDIANA ===
// ========================================================
int medianFilter(int *arr, int n)
{
  int temp[n];

  // copia
  for (int i = 0; i < n; i++)
    temp[i] = arr[i];

  // ordena
  for (int i = 0; i < n - 1; i++)
  {
    for (int j = i + 1; j < n; j++)
    {
      if (temp[j] < temp[i])
      {
        int aux = temp[i];
        temp[i] = temp[j];
        temp[j] = aux;
      }
    }
  }

  // retorna mediana
  if (n % 2 == 1)
    return temp[n / 2];
  else
    return (temp[n / 2] + temp[n / 2 - 1]) / 2;
}

// ========================================================
// === FUNÇÃO DE LEITURA TDS ===
// temperaturaAtual = temperatura da água (°C)
// ========================================================
void lerTDS(float temperaturaAtual)
{

  int readings[NUM_MEDIAN_TDS];

  // coleta leituras
  for (int i = 0; i < NUM_MEDIAN_TDS; i++)
  {
    readings[i] = analogRead(pinTDS);
    delay(20);
  }

  // aplica filtro mediana
  int med = medianFilter(readings, NUM_MEDIAN_TDS);

  // converte em tensão
  float voltage = ((float)med / ADC_MAX) * VREF;

  // polinômio do conversor TDS
  float tdsValue = 133.42f * pow(voltage, 3) - 255.86f * pow(voltage, 2) + 857.39f * voltage;

  // calibração
  tdsValue *= calibrationFactorTDS;

  // compensação da temperatura
  float k = 0.02f;
  float tdsCompensado = tdsValue / (1 + k * (temperaturaAtual - 25.0f));

  // garante valores positivos
  tdsFinal = (tdsCompensado < 0) ? 0 : tdsCompensado;

  // classificação
  if (tdsFinal < 300)
  {
    statusCondutividade = "Baixo";
  }
  else if (tdsFinal < 1000)
  {
    statusCondutividade = "Medio";
  }
  else
  {
    statusCondutividade = "Alto";
  }
}

// ========================================================
// === SETUP ===
// ========================================================
void setup()
{
  Serial.begin(115200);
  delay(500);

  pinMode(pinTDS, INPUT);

  Serial.println("Sensor TDS iniciado!");
}

// ========================================================
// === LOOP ===
// ========================================================
void loop()
{

  // coloque a temperatura real do seu sensor depois
  float temperaturaAtual = 25.0;

  lerTDS(temperaturaAtual);

  Serial.print("TDS: ");
  Serial.print(tdsFinal);
  Serial.print(" ppm | Status: ");
  Serial.println(statusCondutividade);

  delay(500);
}
