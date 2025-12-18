#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ================= CONFIGURAÇÃO DO WIFI =================
const char* ssid = "CSI-Lab";
const char* password = "In@teLCS&I";

// ================= CONFIGURAÇÃO DO MQTT =================
const char* mqtt_server = "192.168.66.11";
const int   mqtt_port   = 1883;
const char* mqtt_user   = "csilab";
const char* mqtt_pass   = "WhoAmI#2024";

const char* esp1_topic = "esp1/sensores";
const char* esp2_topic = "esp2/sensores";

WiFiClient espClient;
PubSubClient client(espClient);

// ================= CONFIGURAÇÃO DO LORA =================
#define LORA_SCK  18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS   5
#define LORA_RST  26
#define LORA_DIO0 2
#define LORA_FREQ 868E6

// ================= VARIÁVEIS =================
String pacoteRecebido = "";

// ================= FUNÇÃO AUXILIAR =================
bool getCampo(String s, String chave, float &dest) {
  int i = s.indexOf(chave);
  if (i < 0) return false;
  i += chave.length();
  int f = s.indexOf(';', i);
  if (f < 0) f = s.length();
  dest = s.substring(i, f).toFloat();
  return true;
}

// ================= CONEXÃO WIFI =================
void setup_wifi() {
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado!");
}

// ================= RECONNECT MQTT =================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ESP32_LoRa_Gateway", mqtt_user, mqtt_pass)) {
      Serial.println(" conectado!");
    } else {
      Serial.print(" falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5s");
      delay(5000);
    }
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Wi-Fi
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("ERRO: LoRa não iniciou");
    while (1);
  }
  Serial.println("LoRa iniciado!");
}

// ================= LOOP =================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Recebe pacote LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    pacoteRecebido = "";
    while (LoRa.available()) {
      pacoteRecebido += (char)LoRa.read();
    }
    pacoteRecebido.trim();
    pacoteRecebido.replace("\r",""); // remove \r se houver

    Serial.println("📡 Pacote recebido: " + pacoteRecebido);

    // Detecta ESP1 ou ESP2
    if (pacoteRecebido.indexOf("CH4:") >= 0) {
      // ESP1
      float ch4=0, nh3=0, co2=0, tvoc=0, dist=0;
      getCampo(pacoteRecebido,"CH4:", ch4);
      getCampo(pacoteRecebido,"NH3:", nh3);
      getCampo(pacoteRecebido,"CO2:", co2);
      getCampo(pacoteRecebido,"TVOC:", tvoc);
      getCampo(pacoteRecebido,"DIST:", dist);

      String payload = "{";
      payload += "\"ch4\":" + String(ch4,0) + ",";
      payload += "\"nh3\":" + String(nh3,0) + ",";
      payload += "\"co2\":" + String(co2,0) + ",";
      payload += "\"tvoc\":" + String(tvoc,0) + ",";
      payload += "\"dist\":" + String(dist,1);
      payload += "}";

      Serial.print("Publicando ESP1: ");
      Serial.println(payload);
      client.publish(esp1_topic, payload.c_str(), true);
    } 
    if (pacoteRecebido.indexOf("TEMP:") >= 0) {
      // ESP2
      float temp=0, tds=0, ntu=0, ph=0;
      getCampo(pacoteRecebido,"TEMP:", temp);
      getCampo(pacoteRecebido,"TDS:", tds);
      getCampo(pacoteRecebido,"NTU:", ntu);
      getCampo(pacoteRecebido,"PH:", ph);

      String payload = "{";
      payload += "\"temp\":" + String(temp,2) + ",";
      payload += "\"tds\":" + String(tds,0) + ",";
      payload += "\"ntu\":" + String(ntu,0) + ",";
      payload += "\"ph\":" + String(ph,2);
      payload += "}";

      Serial.print("Publicando ESP2: ");
      Serial.println(payload);
      client.publish(esp2_topic, payload.c_str(), true);
    }
  }
}
