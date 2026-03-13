#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ver como faz para deixar isso invisivel no github
//  ================= WIFI =================
const char *ssid = "CSI-Lab";
const char *password = "In@teLCS&I";

// ================= MQTT =================
const char *mqtt_server = "192.168.66.11";
const int mqtt_port = 1883;
const char *mqtt_user = "csilab";
const char *mqtt_pass = "WhoAmI#2024";

// ================= LORA =================
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 26
#define LORA_DIO0 2
#define LORA_FREQ 915E6

WiFiClient espClient;
PubSubClient client(espClient);

// ================= FUNÇÃO AUX =================
bool getCampoIntCI(String s, String chave, int &dest)
{
  String s2 = s;
  s2.toUpperCase();
  String c2 = chave;
  c2.toUpperCase();

  int i = s2.indexOf(c2);
  if (i < 0)
    return false;

  i += c2.length();
  int f = s2.indexOf(';', i);
  if (f < 0)
    f = s2.length();

  dest = s2.substring(i, f).toInt();
  return true;
}

// ================= WIFI =================
void setup_wifi()
{
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("conectado!");
}

// ================= MQTT =================
void reconnect_mqtt()
{
  while (!client.connected())
  {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("SMARTMILK_GATEWAY", mqtt_user, mqtt_pass))
    {
      Serial.println("conectado!");
    }
    else
    {
      Serial.print("falhou (");
      Serial.print(client.state());
      Serial.println("), tentando novamente...");
      delay(5000);
    }
  }
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ))
  {
    Serial.println("ERRO: LoRa não iniciou");
    while (1)
      ;
  }

  Serial.println("Gateway SmartMilk iniciado");
}

// ================= LOOP =================
void loop()
{
  if (!client.connected())
  {
    reconnect_mqtt();
  }
  client.loop();

  int packetSize = LoRa.parsePacket();
  if (!packetSize)
    return;

  String pacote = "";
  while (LoRa.available())
  {
    pacote += (char)LoRa.read();
  }

  pacote.trim();
  pacote.replace("\r", "");
  pacote.replace("\n", "");

  Serial.println("LoRa recebido:");
  Serial.println(pacote);

  int tanque = -1;
  if (!getCampoIntCI(pacote, "tanque:", tanque))
  {
    Serial.println("Pacote sem tanque — ignorado");
    return;
  }

  int regiao = -1;
  if (!getCampoIntCI(pacote, "regiao:", regiao))
  {
    Serial.println("Pacote sem regiao — ignorado");
    return;
  }

  // Publica o pacote completo (inclui tanque e regiao), pois o banco precisa dos 2
  String payload = pacote;

  String topic =
      "regiao/" + String(regiao) +
      "/tanque/" + String(tanque) +
      "/sensores";

  bool ok = client.publish(topic.c_str(), payload.c_str(), false);
  if (!ok)
    Serial.println("Falha ao publicar no MQTT");

  Serial.println("MQTT publicado em:");
  Serial.println(topic);
  Serial.println(payload);
}
