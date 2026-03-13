#include <OneWire.h>
#include <DallasTemperature.h>

// ========================================================
// === CONFIGURAÇÕES DO SENSOR
// ========================================================

// Pino de dados do DS18B20
#define PINO_TEMP 27

// Objetos para comunicação com o sensor
OneWire oneWire(PINO_TEMP);
DallasTemperature sensors(&oneWire);

// ========================================================
// === VARIÁVEIS GLOBAIS
// ========================================================

float tempC = 0.0;         // Temperatura em Celsius
String statusTemp = "N/A"; // Status da leitura

// ========================================================
// === FUNÇÃO: LEITURA DA TEMPERATURA
// ========================================================

void lerTemperatura()
{

    // Solicita leitura ao sensor
    sensors.requestTemperatures();

    // Lê a temperatura do primeiro sensor encontrado
    tempC = sensors.getTempCByIndex(0);

    // ----------------------------------------------------
    // Verificação de erro de leitura
    // ----------------------------------------------------

    if (tempC == -127.00)
    {
        // Valor padrão caso o sensor falhe
        tempC = 25.0;
        statusTemp = "Erro";
    }
    else
    {

        // Classificação do status da temperatura
        if (tempC > 0 && tempC <= 25)
        {
            statusTemp = "Boa";
        }
        else
        {
            statusTemp = "Ruim";
        }
    }
}

// ========================================================
// === SETUP
// ========================================================

void setup()
{

    Serial.begin(115200);

    // Inicializa comunicação com o DS18B20
    sensors.begin();

    Serial.println("Leitura do sensor DS18B20 iniciada...");
}

// ========================================================
// === LOOP PRINCIPAL
// ========================================================

void loop()
{

    // ----------------------------------------------------
    // verificar se o DS18B20 está sendo detectado
    // ----------------------------------------------------

    /*
    byte addr[8];
    int count = 0;

    oneWire.reset_search();

    while (oneWire.search(addr)) {

        Serial.print("Encontrado device #");
        Serial.print(count);
        Serial.print(" - endereco: ");

        for (int i = 0; i < 8; i++) {
            if (addr[i] < 16) Serial.print("0");
            Serial.print(addr[i], HEX);
            Serial.print(" ");
        }

        Serial.println();
        count++;
    }

    Serial.print("Total encontrados: ");
    Serial.println(count);

    if (count == 0) {
        Serial.println(">> Nenhum DS18B20 detectado. Verifique VCC/GND/DATA/pull-up.");
    }

    delay(3000);
    */

    // ----------------------------------------------------
    // Leitura da temperatura
    // ----------------------------------------------------

    lerTemperatura();

    // ----------------------------------------------------
    // Exibição no Serial Monitor
    // ----------------------------------------------------

    Serial.print("Temperatura: ");
    Serial.print(tempC);
    Serial.print(" °C | Status: ");
    Serial.println(statusTemp);

    // Atualização a cada 2 segundos
    delay(2000);
}