// ===== ESP32 B COMPLETO: MQTT + LCD + pH + Temp + Turbidez + TDS =====
// Foco na estabilidade da comunicação e correções de lógica nos sensores.

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ===== CONSTANTES GLOBAIS =====
const float VREF = 3.3f;
const int ADC_MAX = 4095;
const int NUM_SAMPLES_TURB = 50; // REDUZIDO de 800 para 50 para evitar travamentos
const int NUM_MEDIAN_TDS = 7;    // Número de amostras para filtro de TDS

// ===== CONFIGURAÇÕES WIFI =====
const char* ssid = "CSI-Lab";
const char* password = "In@teLCS&I";

// ===== CONFIGURAÇÕES MQTT =====
const char* mqttServer = "192.168.66.50"; 
const int mqttPort = 1883;
const char* mqttUser = "csilab";
const char* mqttPassword = "WhoAmI#2023";
const char* topicPublish = "Baixo/Sensores";
const char* topicSubscribe = "Cima/Sensores";

WiFiClient espClient;
PubSubClient client(espClient);

// ===== PINOS DOS SENSORES =====
#define ONE_WIRE_BUS 27    // DS18B20
const int pinPh = 32;      // Sensor de pH
const int pinTurbidez = 33;// Sensor de Turbidez 
const int pinTDS = 34;     // Sensor TDS

// ===== OBJETOS =====
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
// LCD Display ST7920 (SPI Software: Clock=18, Data=4, CS=5, Reset=26)
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, 18, 4, 5, 26);

// ===== VARIÁVEIS GLOBAIS DE ESTADO =====
// -- pH e Temp --
float PhFinal = 0;
float tempC = 0;
String statusTemp = "N/A";
String statusPh = "N/A";

// -- Turbidez --
float ntuFinal = 0;
String statusTurbidez = "N/A";

// -- TDS/Condutividade --
float tdsFinal = 0;
float calibrationFactorTDS = 1.00f; // Ajuste conforme calibração
String statusCondutividade = "N/A"; // NOVO: Variável para status de Condutividade

// ===== FUNÇÕES AUXILIARES DE PROCESSAMENTO =====

// Função para arredondar
float ArredondarPara(float ValorEntrada, int CasaDecimal) {
  float multiplicador = powf(10.0f, CasaDecimal);
  return roundf(ValorEntrada * multiplicador) / multiplicador;
}

// Filtro Mediano (max_size 21 para o TDS)
int medianFilter(int samples[], int size) {
  // CORRIGIDO: O array temporário deve ser grande o suficiente.
  int temp[21]; 
  if (size > 21) size = 21; // Garantir que não exceda o buffer
  
  for (int i = 0; i < size; ++i) temp[i] = samples[i];
  
  // Bubble Sort
  for (int i = 0; i < size-1; ++i) {
    for (int j = 0; j < size-1-i; ++j) {
      if (temp[j] > temp[j+1]) {
        int t = temp[j]; temp[j] = temp[j+1]; temp[j+1] = t;
      }
    }
  }
  return temp[size/2];
}

// ===== FUNÇÕES DE LEITURA DE SENSORES =====

// Leitura de pH (CORRIGIDO: Fórmula de calibração)
void lerPh() {
  float leituraAnalogPh = analogRead(pinPh);
  float voltagemPH = leituraAnalogPh * (VREF / ADC_MAX);
  
  // CORREÇÃO DA FÓRMULA DE PH: 
  // Assumindo calibração linear: pH = 7.0 + ((V_Meio - V_Lido) / Slope)
  // Onde V_Meio (pH 7.0) é tipicamente 1.65V (metade de 3.3V) e Slope é ~0.18V/pH.
  PhFinal = 7.0 + ((1.65f - voltagemPH) / 0.18f); 
  PhFinal = ArredondarPara(PhFinal, 2);

  statusPh = (PhFinal >= 6.4 && PhFinal <= 7.0) ? "Adequado" : "Inadequado";
}

// Leitura Turbidez (CORRIGIDO: Redução das amostras)
void lerTurbidez() {
  float voltagem = 0;
  for (int i = 0; i < NUM_SAMPLES_TURB; i++) {
    voltagem += (analogRead(pinTurbidez) / (float)ADC_MAX) * VREF;
  }
  // CORRIGIDO: Divisão correta pela quantidade de amostras
  voltagem = voltagem / (float)NUM_SAMPLES_TURB;
  voltagem = ArredondarPara(voltagem, 2);

  float ntu = 0;
  // Polinômio baseado no sensor (mantido, mas pode precisar de ajuste fino)
  if (voltagem < 2.5) {
    ntu = 3000;
  } else if (voltagem > 4.2) {
    ntu = 0;
  } else {
    ntu = -1120.4 * pow(voltagem, 2) + 5742.3 * voltagem - 4353.8;
  }
  ntuFinal = (ntu < 0) ? 0 : ntu;

  if (ntuFinal <= 5) statusTurbidez = "Limpa";
  else if (ntuFinal <= 50) statusTurbidez = "Media";
  else statusTurbidez = "Suja";
}


// Leitura TDS com Filtro e Compensação
void lerTDS(float temperaturaAtual) {
  int readings[NUM_MEDIAN_TDS];
  
  for (int i = 0; i < NUM_MEDIAN_TDS; ++i) {
    readings[i] = analogRead(pinTDS);
    delay(20); // Pequeno delay entre amostras
  }
  
  int med = medianFilter(readings, NUM_MEDIAN_TDS); 
  float voltage = ((float)med / (float)ADC_MAX) * VREF;
  
  // Converte tensão em TDS (ppm) - Polinômio
  float tdsValue = 133.42f * powf(voltage, 3.0f) - 255.86f * powf(voltage, 2.0f) + 857.39f * voltage;
  tdsValue *= calibrationFactorTDS;
  
  // Compensação de temperatura (k=0.02f)
  float k = 0.02f;
  float tdsCompensado = tdsValue / (1.0f + k * (temperaturaAtual - 25.0f));
  
  tdsFinal = (tdsCompensado < 0) ? 0 : tdsCompensado;
}


// ===== FUNÇÕES DE COMUNICAÇÃO E DISPLAY =====

void publishMQTT() {
  String payload = "{";
  payload += "\"ph\":" + String(PhFinal, 2) + ",";
  payload += "\"temperatura\":" + String(tempC, 1) + ",";
  // CORRIGIDO: Envia o valor do TDS no campo condutividade
  payload += "\"condutividade\":\"" + String(tdsFinal, 0) + "\","; 
  payload += "\"turbidez\":\"" + String(ntuFinal, 1) + "\","; 
  payload += "\"statusPh\":\"" + statusPh + "\",";
  payload += "\"statusTemp\":\"" + statusTemp + "\",";
  // CORRIGIDO: Envia o status da Condutividade/TDS
  payload += "\"statusCondutividade\":\"" + statusCondutividade + "\","; 
  payload += "\"statusTurbidez\":\"" + statusTurbidez + "\"";
  payload += "}";

  client.publish(topicPublish, payload.c_str());
  Serial.println("Publicado: " + payload);
}

void draw() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);

  // Alterna visualização a cada 3 segundos
  if (millis() / 3000 % 2 == 0) {
    u8g2.drawStr(0, 10, "--- AGUA (1/2) ---");
    
    u8g2.drawStr(0, 25, "pH: ");
    u8g2.setCursor(30, 25); u8g2.print(PhFinal, 2);
    
    u8g2.drawStr(0, 35, "Sts: ");
    u8g2.setCursor(30, 35); u8g2.print(statusPh);

    u8g2.drawStr(0, 50, "Tmp: ");
    u8g2.setCursor(30, 50); u8g2.print(tempC, 1);
    u8g2.print(" C");
  } else {
    u8g2.drawStr(0, 10, "--- QUALID (2/2) --");
    
    u8g2.drawStr(0, 25, "Turb: ");
    u8g2.setCursor(35, 25); u8g2.print(ntuFinal, 0);
    u8g2.print(" NTU");

    u8g2.drawStr(0, 40, "TDS:  ");
    u8g2.setCursor(35, 40); u8g2.print(tdsFinal, 0);
    u8g2.print(" ppm");
    
    u8g2.drawStr(0, 55, "Qual: ");
    u8g2.setCursor(35, 55); u8g2.print(statusCondutividade); // Usa o novo status
  }
  u8g2.sendBuffer();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Msg recebida ["); Serial.print(topic); Serial.print("]: ");
  for (unsigned int i = 0; i < length; i++) { Serial.print((char)payload[i]); }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");
    if (client.connect("ESP32B_Sensors", mqttUser, mqttPassword)) {
      Serial.println("OK!");
      client.subscribe(topicSubscribe);
    } else {
      Serial.print("Falha rc="); Serial.print(client.state());
      Serial.println(" tenta em 2s");
      delay(2000);
    }
  }
}


// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  
  // Configuração ADC
  analogReadResolution(12);
  // Define a atenuação para ler a faixa completa de 0 a 3.3V.
  analogSetPinAttenuation(pinTDS, ADC_11db); 
  analogSetPinAttenuation(pinPh, ADC_11db); 
  analogSetPinAttenuation(pinTurbidez, ADC_11db); 

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" OK!");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // Inicialização Sensores
  sensors.begin();

  // Inicialização LCD
  u8g2.begin();
  u8g2.clearDisplay();
  Serial.println("Sistema Iniciado.");
}

// ===== LOOP =====
void loop() {
  // 1. Manutenção de Conexão (Chave para comunicação estável)
  if (!client.connected()) reconnect();
  client.loop();

  // 2. LEITURA DE TEMPERATURA (necessária antes do TDS)
  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);
  if(tempC == -127.00) tempC = 25.0; // Fallback se sensor falhar
  statusTemp = (tempC > 0 && tempC <= 25) ? "Boa" : "Ruim";

  // 3. LEITURA DE TDS
  lerTDS(tempC);

  // Lógica de Status de Condutividade/TDS
  // Valores de referência para leite ou água com sólidos dissolvidos
  if (tdsFinal < 300) { 
    statusCondutividade = "Baixo"; 
  } else if (tdsFinal < 1000) {
    statusCondutividade = "Medio";
  } else {
    statusCondutividade = "Alto";
  }

  // 4. LEITURA DE TURBIDEZ
  lerTurbidez();

  // 5. LEITURA DE PH
  lerPh();

  // 6. PUBLICAR JSON Completo
  publishMQTT();

  // 7. ATUALIZAR LCD E DELAY (Delay de 2s para estabilidade do MQTT)
  draw(); 
  delay(2000); 
}