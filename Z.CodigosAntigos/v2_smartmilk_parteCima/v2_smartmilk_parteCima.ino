// ===== ESP32 A =====
// Publica os sensores locais em JSON no tópico "Cima/Sensores"
//ultrassonico, gas co2, gas 135 (amonia), gas mq2 (metano), lora 
// Assina JSON em "Baixo/Sensores" (pH, temperatura, condutividade, turbidez, status) e envia TUDO no LoRa.

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <EEPROM.h>

// ===== CONFIGURAÇÕES WIFI =====
const char* ssid     = "TP-LINK_3A04"; //nome roteador 
const char* password = "27453148"; //senha roteador

// ===== CONFIGURAÇÕES MQTT =====
const char* mqttServer   = "192.168.12.103";
const int   mqttPort     = 1883;
const char* mqttUser     = "admin";
const char* mqttPassword = "admin";

const char* topicPublish   = "Cima/Sensores";  // A -> publica dados locais (JSON)
const char* topicSubscribe = "Baixo/Sensores"; // A <- assina pH/Temp/Status (JSON)
const char* mqttClientId   = "ESP32A_smartmilk_12";

// ===== PINOS DOS SENSORES =====
//ultrassonico AJ-SR04M -> (distancia minima 25cm distancia maxima 450cm)
#define PIN_TRIG   14
#define PIN_ECHO   4

// MQ-2 (metano) - pino analógico
#define PIN_MQ2    34

// MQ-135 (amônia) - pino analógico
#define PIN_MQ135  35

// Sensor CO2 (co2) - pino analógico 
#define PIN_CO2    32

// ===== CONFIGURAÇÕES DO SENSOR ULTRASSONICO (MÉDIA MÓVEL) =====
const unsigned long TIMEOUT_US = 30000UL; // timeout 30ms
const float SOUND_CM_PER_US = 0.0343f;    // Velocidade som
const int NUM_SAMPLES = 5;                // Quantidade de leituras para a média

// buffer média móvel
float samples[NUM_SAMPLES];
int sampleIndex = 0;
bool samplesFilled = false;

// ===== LoRa =====
// fiação: gnd,sck(18),miso(19),mosi(23),nss(SS),dio0(DIO0),vcc,reset(RESET)
#define SS    5
#define DIO0  27
#define RESET 26
#define LORA_FREQ 915E6   // BR

// ===== METADADOS =====
//tanque da barbara
int id_tanque = 29;
int id_regiao = 20;
float raio_cm = 9.0f;           // cm
float altura_tanque_cm = 18.5f;   // cm


// ===== VARIÁVEIS LOCAIS =====
float metano = 0.0f;
int co2_adc = 0;           // leitura bruta (analógica) do CO2 
float amonia = 0.0f;
float distancia = 0.0f;
float altura_agua = 0.0f;
float litros = 0.0f;

// ===== VARIÁVEIS ASSINADAS (remotas) =====
String dadoPh = "N/A";
String dadoTemperatura = "N/A";

String dadoStatusPh = "N/A";
String dadoStatusTemperatura = "N/A";

String dadoCondutividade = "N/A";
String dadosStatusCondutividade = "N/A";

String dadosTurbidez = "N/A";
String dadosStatusTurbidez = "N/A";


// ===== MQTT/WIFI =====
WiFiClient espClient;
PubSubClient client(espClient);

// ===== UTILS =====
static float clampFloorZero(float v) { return (v < 0) ? 0.0f : v; }

// ---- Parser JSON bem simples (sem ArduinoJson) ----
String jsonGet(const String& js, const char* key) {
  String kq = String("\"") + key + "\"";
  int p = js.indexOf(kq);
  if (p < 0) return "";

  // acha ':' depois da chave
  int colon = js.indexOf(':', p + kq.length());
  if (colon < 0) return "";

  // pula espaços
  int i = colon + 1;
  while (i < (int)js.length() && (js[i] == ' ' || js[i] == '\t')) i++;

  // string entre aspas?
  if (i < (int)js.length() && js[i] == '\"') {
    int ini = i + 1;
    int fim = js.indexOf('\"', ini);
    if (fim < 0) return "";
    return js.substring(ini, fim);
  }

  // pega até vírgula ou fim/fecha
  int fim = js.indexOf(',', i);
  int close = js.indexOf('}', i);
  if (fim < 0 || (close >= 0 && close < fim)) fim = close;
  if (fim < 0) fim = js.length();
  String raw = js.substring(i, fim);
  raw.trim();
  return raw;
}

// ===== FUNÇÕES DO SENSOR ULTRASSONICO =====

float measureDistanceCm() {
  // Gera pulso TRIG (Usando seus pinos definidos lá em cima: 14 e 4)
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); 
  delayMicroseconds(10); 
  digitalWrite(PIN_TRIG, LOW); 

  // Lê pulso ECHO
  unsigned long duration = pulseIn(PIN_ECHO, HIGH, TIMEOUT_US); 

  if (duration == 0) {
    return -1.0f; // timeout / erro
  }

  // Calcula distância em cm
  return (duration * SOUND_CM_PER_US) / 2.0f;
}

float getSmoothedDistance() {
  float d = measureDistanceCm();
  if (d >= 0.0f) {
    samples[sampleIndex] = d;
    sampleIndex++;
    if (sampleIndex >= NUM_SAMPLES) { sampleIndex = 0; samplesFilled = true; }
  } else {
    // se falhar e ainda não houver amostras, retornamos -1
    if (!samplesFilled && sampleIndex == 0) return -1.0f;
  }

  // Calcula média
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
// ==================================

// ===========================
// === FUNÇÕES SENSORES ANALÓGICOS
// ===========================
// Observação: ESP32 analogRead retorna 0..4095 por padrão.
// Se quiser valores de tensão, calcule: v = raw*(3.3/4095) (ou use atenuação ADC)

// Mapeamento simples para MQ-135: converte leitura ADC para "classificação" 0..100
// AGORA USADO APENAS PARA AMÔNIA (NH3)
float classifyMQ135(int raw) {
  if (raw <= 150) return 10.0f;
  else if (raw <= 400) return 30.0f;
  else if (raw <= 700) return 50.0f;
  else return 100.0f;
}

// Mapeamento simples para MQ-2
float classifyMQ2(int raw) {
  if (raw <= 150) return 10.0f;
  else if (raw <= 400) return 30.0f;
  else if (raw <= 700) return 50.0f;
  else return 100.0f;
}

// Mapeamento simples para CO2 analógico (ajuste se tiver calibração)
float classifyCO2(int raw) {
  // exemplo: escala 0..4095 -> 0..100 (não é PPM real)
  float pct = (raw / 4095.0f) * 100.0f;
  return pct; // use como "intensidade" relativa
}


// ===== FUNÇÃO CALLBACK MQTT =====
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  String message;
  message.reserve(length + 8);
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  message.trim();

  Serial.print("Conteúdo: ");
  Serial.println(message);

  // Esperado JSON
  String phS   = jsonGet(message, "ph");
  String tS    = jsonGet(message, "temperatura");
  String sPh   = jsonGet(message, "statusPh");
  String sTemp = jsonGet(message, "statusTemp");
  String cond  = jsonGet(message, "condutividade");
  String sCond = jsonGet(message, "statusCondutividade");
  String turb  = jsonGet(message, "turbidez");
  String sTurb = jsonGet(message, "statusTurbidez");

  if (phS.length())    dadoPh = phS;
  if (tS.length())     dadoTemperatura = tS;
  if (sPh.length())    dadoStatusPh = sPh;
  if (sTemp.length())  dadoStatusTemperatura = sTemp;
  if (cond.length())   dadoCondutividade = cond;
  if (sCond.length())  dadosStatusCondutividade = sCond;
  if (turb.length())   dadosTurbidez = turb;
  if (sTurb.length())  dadosStatusTurbidez = sTurb;
}

// ===== FUNÇÃO PARA RECONEXÃO MQTT =====
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect(mqttClientId, mqttUser, mqttPassword)) {
      Serial.println("Conectado!");
      client.subscribe(topicSubscribe);
      Serial.print("Assinado: ");
      Serial.println(topicSubscribe);
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 2s");
      delay(2000);
    }
  }
}

// ===== WIFI =====
void connectWiFi() {
  Serial.print("Conectando ao WiFi ");
  Serial.print(ssid);
  Serial.println(" ...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.print("\nWiFi OK. IP: ");
  Serial.println(WiFi.localIP());
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

 // pinos sensores
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  // Inicializa buffer do novo sensor com zeros
  for (int i = 0; i < NUM_SAMPLES; ++i) samples[i] = 0.0f;

  // WiFi + MQTT
  connectWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // LoRa
  Serial.println("Iniciando LoRa...");
  LoRa.setPins(SS, RESET, DIO0);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("Falha ao iniciar LoRa (seguindo sem travar).");
  } else {
    Serial.println("LoRa OK.");
  }
}

// ===== LOOP =====
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // ===== LEITURA dos sensores (locais) =====
  int raw_mq135 = analogRead(PIN_MQ135); 
  int raw_mq2   = analogRead(PIN_MQ2);   
  int raw_co2   = analogRead(PIN_CO2);   

  // Classificações simples (ajuste conforme calibração real)
  amonia = classifyMQ135(raw_mq135); // MQ-135 mede APENAS amônia
  metano = classifyMQ2(raw_mq2);
  co2_adc = raw_co2; 
  float co2_class = classifyCO2(raw_co2); 


  // >>> LÓGICA ULTRASSÔNICO <<<
  float leituraNova = getSmoothedDistance();
  
  // Só atualiza a variável principal se a leitura for válida (>=0)
  if (leituraNova >= 0.0f) {
       distancia = leituraNova;
  } else {
       Serial.println("Erro leitura ultrassônico (timeout)");
       // Se der erro, mantemos o último valor válido de 'distancia'
  }
  
  // --- CALCULO VOLUME
  altura_agua = clampFloorZero(altura_tanque_cm - distancia);
  float volume_cm3 = 3.14159f * raio_cm * raio_cm * altura_agua; // cm³
  litros = volume_cm3 / 1000.0f;    //L       

  // ===== PUBLICAR EM JSON (locais) =====
  String payload = "{";
  payload += "\"mq135\":" + String(raw_mq135) + ","; 
  payload += "\"co2_adc\":" + String(raw_co2) + ","; 
  payload += "\"co2_class\":" + String(co2_class,1) + ",";
  payload += "\"amonia\":" + String(amonia,1) + ",";
  payload += "\"mq2\":" + String(raw_mq2) + ","; 
  payload += "\"metano\":" + String(metano,1) + ",";
  payload += "\"distancia\":" + String(distancia,2) + ",";
  payload += "\"altura_agua\":" + String(altura_agua,2) + ",";
  payload += "\"volume_l\":" + String(litros,2);
  payload += "}";

  client.publish(topicPublish, payload.c_str());
  Serial.println("Publicado (Cima/Sensores): " + payload);

  // ===== ENVIAR TUDO NO LoRa (locais + assinados) =====
  // Formato estilo chave:valor|... (compatível com seu receptor atual)

  String mensagem = "mimosa\n";
  mensagem += "ID_TANQUE:" + String(id_tanque) + "|";
  mensagem += "ID_REGIAO:" + String(id_regiao) + "|";

  // Locais
  mensagem += "mq-135:" + String(raw_mq135) + "|"; 
  mensagem += "co2_adc:" + String(raw_co2) + "|"; 
  mensagem += "co2_class:" + String(co2_class,1) + "|";
  mensagem += "amonia:" + String(amonia,1) + "|";
  mensagem += "mq-2:" + String(raw_mq2) + "|"; 
  mensagem += "metano:" + String(metano,1) + "|";
  mensagem += "distancia:" + String(distancia,2) + "|";
  mensagem += "altura:" + String(altura_agua,2) + "|";
  mensagem += "volume:" + String(litros,2) + "|";

  // Remotos (vindos do SUB em JSON)
  mensagem += "temperatura:" + (dadoTemperatura.length() ? dadoTemperatura : "N/A") + "|";
  mensagem += "status_temp:" + (dadoStatusTemperatura.length() ? dadoStatusTemperatura : "N/A") + "|";
  mensagem += "ph:" + (dadoPh.length() ? dadoPh : "N/A") + "|";
  mensagem += "status_ph:" + (dadoStatusPh.length() ? dadoStatusPh : "N/A") + "|";
  mensagem += "condutividade:" + (dadoCondutividade.length() ? dadoCondutividade : "N/A") + "|";
  mensagem += "status_condutividade:" + (dadosStatusCondutividade.length() ? dadosStatusCondutividade : "N/A") + "|";
  mensagem += "turbidez:" + (dadosTurbidez.length() ? dadosTurbidez : "N/A") + "|";
  mensagem += "status_turbidez:" + (dadosStatusTurbidez.length() ? dadosStatusTurbidez : "N/A");

  LoRa.beginPacket();
  LoRa.print(mensagem);
  LoRa.endPacket();

  Serial.println("LoRa enviado:");
  Serial.println(mensagem);

  delay(5000);
}