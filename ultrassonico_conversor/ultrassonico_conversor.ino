#define TRIG 14        // TRIG direto no ESP32
#define ECHO 27        // ECHO no level shifter LV → HV → Sensor

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);
}

void loop() {

  // ---- Disparo do TRIG ----
  digitalWrite(TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  // ---- Leitura com timeout ----
  long duracao = pulseIn(ECHO, HIGH, 30000); // 30 ms timeout

  // ---- Diagnósticos ----

  if (duracao == 0) {  
    Serial.println("ERRO: ECHO sem pulso (desconectado, errado ou power fraco)");
    delay(300);
    return;
  }

  if (duracao < 100) {  
    Serial.println("AVISO: Pulso muito curto (sensor piscando ou defeito)");
    delay(300);
    return;
  }

  // ---- Cálculo ----
  float distancia = duracao * 0.0343 / 2;

  if (distancia < 2 || distancia > 450) {
    Serial.print("Fora do alcance: ");
    Serial.print(distancia);
    Serial.println(" cm");
  } 
  else {
    Serial.print("Distancia: ");
    Serial.print(distancia);
    Serial.println(" cm");
  }

  delay(300);
}
