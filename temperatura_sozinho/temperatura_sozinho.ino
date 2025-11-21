// ========================================================
// === MÓDULO 2: SENSOR DE TEMPERATURA (DS18B20)
// ========================================================

// --- PINOS E OBJETOS ---
#define ONE_WIRE_BUS 27
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// --- VARIÁVEIS DE ESTADO ---
float tempC = 0;
String statusTemp = "N/A";

// --- FUNÇÃO DE LEITURA ---
void lerTemperatura() {
  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);
  
  // Fallback para temperatura se a leitura falhar
  if(tempC == -127.00) tempC = 25.0; 
  
  // Classificação do Status
  statusTemp = (tempC > 0 && tempC <= 25) ? "Boa" : "Ruim";
}