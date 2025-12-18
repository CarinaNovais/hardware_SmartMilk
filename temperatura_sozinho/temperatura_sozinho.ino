#include <OneWire.h>
#include <DallasTemperature.h>

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
    if (tempC == -127.00) {
        tempC = 25.0;
        statusTemp = "Erro";
    } else {
        // Classificação do Status
        statusTemp = (tempC > 0 && tempC <= 25) ? "Boa" : "Ruim";
    }
}

// ========================================================
// === SETUP E LOOP
// ========================================================
void setup() {
    Serial.begin(115200);
    sensors.begin();
    Serial.println("Leitura do sensor DS18B20 iniciada...");
}

void loop() {
    lerTemperatura();

    // Exibe no Serial
    Serial.print("Temperatura: ");
    Serial.print(tempC);
    Serial.print(" °C | Status: ");
    Serial.println(statusTemp);

    delay(2000); // Atualiza a cada 2 segundos
}
