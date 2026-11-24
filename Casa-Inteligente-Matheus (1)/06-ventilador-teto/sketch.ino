// ---------------------------------------------------------------------------
// Casa Inteligente | 06 | Ventilador de teto com velocidade automática
//
// O que faz: no modo automático, escolhe a velocidade do ventilador (0 a 3)
// pela sensação térmica, calculada a partir da temperatura e da umidade.
// O botão passa pelos modos Auto -> 1 -> 2 -> 3 -> Desligado -> Auto.
//
// Entradas : sensor DHT22 (temperatura e umidade) e botão de modo.
// Saídas   : display de 7 segmentos com a velocidade (ponto aceso = modo
//            automático) e o motor do ventilador, representado por um LED
//            com brilho proporcional à velocidade (PWM).
// ---------------------------------------------------------------------------
#include <DHT.h>

const uint8_t pino_dht   = 2;
const uint8_t pino_botao = 3;   // para o GND, pull-up interno
const uint8_t pino_motor = 9;   // PWM
// segmentos a, b, c, d, e, f, g e ponto do display (catodo comum)
const uint8_t segmentos[8] = {4, 5, 6, 7, 8, 10, 11, 12};

// padrões dos dígitos 0 a 3 (bit 0 = segmento a ... bit 6 = segmento g)
const uint8_t DIGITOS[4] = {0b0111111, 0b0000110, 0b1011011, 0b1001111};
const uint8_t PWM_POR_VELOCIDADE[4] = {0, 90, 170, 255};

// limites de sensação térmica (°C) para cada velocidade e histerese
const float limites[3] = {24.0, 26.5, 29.0};
const float histerese = 0.5;

enum Modo : uint8_t { AUTOMATICO, MANUAL_1, MANUAL_2, MANUAL_3, DESLIGADO };

DHT sensor(pino_dht, DHT22);
Modo modo = AUTOMATICO;
uint8_t velocidade = 0;
float sensacao = NAN;
unsigned long ultima_leitura = 0;
bool botao_antes = HIGH;
unsigned long botao_quando = 0;

// Sensação térmica simplificada: a umidade alta "esquenta" o ambiente
float calcular_sensacao(float t, float ur) {
  if (ur <= 60) return t;
  return t + (ur - 60) * 0.05;   // +0,5 °C a cada 10 % acima de 60 %
}

// Escolhe a velocidade com histerese: sobe ao passar do limite, desce só
// quando fica meio grau abaixo dele
uint8_t velocidade_automatica(float s, uint8_t atual) {
  uint8_t v = atual;
  while (v < 3 && s >= limites[v]) v++;
  while (v > 0 && s < limites[v - 1] - histerese) v--;
  return v;
}

void mostrar(uint8_t numero, bool ponto) {
  for (uint8_t i = 0; i < 7; i++) digitalWrite(segmentos[i], (DIGITOS[numero] >> i) & 1);
  digitalWrite(segmentos[7], ponto);
}

void aplicar(uint8_t v) {
  if (v != velocidade) {
    velocidade = v;
    Serial.print("[VENTILADOR] Velocidade ");
    Serial.println(v);
  }
  analogWrite(pino_motor, PWM_POR_VELOCIDADE[velocidade]);
  mostrar(velocidade, modo == AUTOMATICO);
}

void setup() {
  Serial.begin(9600);
  pinMode(pino_botao, INPUT_PULLUP);
  pinMode(pino_motor, OUTPUT);
  for (uint8_t p : segmentos) pinMode(p, OUTPUT);
  sensor.begin();
  aplicar(0);
  ultima_leitura = millis() - 2000;   // primeira leitura logo no início
}

void loop() {
  unsigned long agora = millis();

  // Botão: cada toque avança um modo
  bool b = digitalRead(pino_botao);
  if (b != botao_antes && agora - botao_quando > 50) {
    botao_quando = agora;
    botao_antes = b;
    if (b == LOW) {
      modo = (Modo)((modo + 1) % 5);
      Serial.print("[VENTILADOR] Modo ");
      Serial.println(modo == AUTOMATICO ? "automatico" : modo == DESLIGADO ? "desligado" : "manual");
    }
  }

  // Leitura do sensor a cada 2 s (limite do DHT22)
  if (agora - ultima_leitura >= 2000) {
    ultima_leitura = agora;
    float t = sensor.readTemperature(), ur = sensor.readHumidity();
    if (!isnan(t) && !isnan(ur)) {
      sensacao = calcular_sensacao(t, ur);
      Serial.print("[VENTILADOR] T=");
      Serial.print(t, 1);
      Serial.print(" C UR=");
      Serial.print(ur, 0);
      Serial.print(" % sensacao=");
      Serial.println(sensacao, 1);
    }
  }

  // Decisão da velocidade conforme o modo
  uint8_t v;
  switch (modo) {
    case AUTOMATICO: v = isnan(sensacao) ? 1 : velocidade_automatica(sensacao, velocidade); break;  // sem leitura: velocidade 1 por segurança
    case MANUAL_1:   v = 1; break;
    case MANUAL_2:   v = 2; break;
    case MANUAL_3:   v = 3; break;
    default:         v = 0; break;
  }
  aplicar(v);
}
