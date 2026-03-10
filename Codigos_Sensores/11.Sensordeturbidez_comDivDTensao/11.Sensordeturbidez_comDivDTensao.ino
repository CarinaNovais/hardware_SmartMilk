/*
 * Código para Sensor de Turbidez no ESP32
 * Autor: Gemini
 */

// Definição do pino (GPIO 34 é um pino apenas de entrada, ideal para ADC)
const int pinoSensor = 33;

void setup() {
  // Inicia a comunicação serial
  Serial.begin(115200);
  
  // Configura a resolução do ADC para 12 bits (0 - 4095)
  analogReadResolution(12);
  
  // Opcional: Aumenta a atenuação para ler até ~3.3V (padrão no ESP32)
  analogSetAttenuation(ADC_11db);

  Serial.println("Inicializando Sensor de Turbidez ST100...");
  delay(1000);
}

void loop() {
  // 1. Leitura do valor bruto (0 a 4095)
  int valorAnalogico = analogRead(pinoSensor);

  // 2. Conversão para Tensão (Voltagem)
  // 3.3V é a referência, 4095 é a resolução máxima
  float voltagem = valorAnalogico * (3.3 / 4095.0);

  // 3. Lógica de Turbidez (NTU)
  // NOTA: Sensores de turbidez funcionam de forma inversa:
  // Alta voltagem (ex: 4V ou 3V) = Água Limpa
  // Baixa voltagem (ex: 2V ou 1V) = Água Suja (a luz não passa)
  
  String qualidade = "";
  
  if (voltagem > 2.5) {
    qualidade = "Limpa";
  } else if (voltagem > 1.0) {
    qualidade = "Turva";
  } else {
    qualidade = "Muito Suja";
  }

  // 4. Exibir no Monitor Serial
  Serial.print("Leitura Bruta: ");
  Serial.print(valorAnalogico);
  Serial.print(" | Voltagem: ");
  Serial.print(voltagem);
  Serial.print("V | Status: ");
  Serial.println(qualidade);

  delay(1000); // Aguarda 1 segundo entre leituras
}