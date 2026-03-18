// ===============================================
// === SENSOR MQ-2 (METANO)
// ===============================================

// --- PINO ---
#define PIN_MQ2 34

// --- VARIÁVEIS ---
int raw_mq2 = 0;
float metano = 0.0f;

// --- FUNÇÃO DE CLASSIFICAÇÃO ---
float classifyMQ2(int raw)
{
    if (raw <= 150) return 10.0f;
    else if (raw <= 400) return 30.0f;
    else if (raw <= 700) return 50.0f;
    else return 100.0f;
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("Teste Sensor MQ2 iniciado");

    analogReadResolution(12); // ESP32 -> 0 a 4095
}

void loop()
{
    // leitura do sensor
    raw_mq2 = analogRead(PIN_MQ2);

    // classifica metano
    metano = classifyMQ2(raw_mq2);

    // ===== SERIAL =====
    Serial.println("------ MQ2 ------");

    Serial.print("ADC bruto: ");
    Serial.println(raw_mq2);

    Serial.print("Metano (% estimado): ");
    Serial.println(metano);

    Serial.println("-----------------\n");

    delay(1000);
}