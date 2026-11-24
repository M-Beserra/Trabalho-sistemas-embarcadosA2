// ---------------------------------------------------------------------------
// Casa Inteligente | 03 | Luz do corredor com presença e luz de vigília
//
// O que faz: à noite, o corredor fica com uma luz fraca de vigília (10 %).
// Quando alguém passa, a luz acende em 100 % com subida suave, fica acesa
// enquanto houver movimento e apaga devagar 20 s depois do último movimento.
// De dia a lâmpada fica apagada, mesmo com movimento.
//
// Entradas : sensor de presença PIR e sensor de luminosidade (LDR).
// Saída    : lâmpada dimerizável (LED no pino PWM 9).
//
// Dia ou noite: leitura do LDR com histerese. Acima de 600 (cerca de 60 lux)
// passa a ser noite; abaixo de 520 (cerca de 100 lux) volta a ser dia.
// ---------------------------------------------------------------------------

const uint8_t pino_pir   = 2;
const uint8_t pino_ldr   = A0;
const uint8_t pino_lampada = 9;

const int limiar_noite = 600;
const int limiar_dia   = 520;
const uint8_t brilho_vigilia = 26;     // 10 % de 255
const uint8_t brilho_total   = 255;
const unsigned long manter_acesa_ms = 20000;  // na prática, 2 minutos
const unsigned long degrau_ms = 8;            // velocidade da rampa de brilho

bool noite = false;
unsigned long ultimo_movimento = 0;
bool ja_houve_movimento = false;
int brilho_atual = 0;
int brilho_alvo = 0;
unsigned long ultimo_degrau = 0;

// Atualiza "noite" só quando a leitura cruza um dos limiares (histerese)
void avaliar_luz_ambiente() {
  int leitura = analogRead(pino_ldr);   // mais escuro = valor maior
  bool antes = noite;
  if (!noite && leitura > limiar_noite) noite = true;
  else if (noite && leitura < limiar_dia) noite = false;
  if (noite != antes) {
    Serial.print("[CORREDOR] Agora e ");
    Serial.println(noite ? "NOITE" : "DIA");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(pino_pir, INPUT);
  pinMode(pino_lampada, OUTPUT);
  avaliar_luz_ambiente();
}

void loop() {
  unsigned long agora = millis();
  avaliar_luz_ambiente();

  if (digitalRead(pino_pir) == HIGH) {
    if (!ja_houve_movimento || agora - ultimo_movimento > 1000) Serial.println("[CORREDOR] Movimento");
    ultimo_movimento = agora;
    ja_houve_movimento = true;
  }
  bool alguem_passou = ja_houve_movimento && agora - ultimo_movimento < manter_acesa_ms;

  // Regra de decisão do brilho desejado
  int novo_alvo;
  if (!noite) novo_alvo = 0;                       // dia: apagada
  else if (alguem_passou) novo_alvo = brilho_total; // noite com presença
  else novo_alvo = brilho_vigilia;                  // noite sem presença
  if (novo_alvo != brilho_alvo) {
    brilho_alvo = novo_alvo;
    Serial.print("[CORREDOR] Brilho alvo: ");
    Serial.print(brilho_alvo * 100 / 255);
    Serial.println(" %");
  }

  // Rampa: aproxima o brilho do alvo um degrau por vez (acende e apaga suave)
  if (brilho_atual != brilho_alvo && agora - ultimo_degrau >= degrau_ms) {
    ultimo_degrau = agora;
    brilho_atual += (brilho_alvo > brilho_atual) ? 1 : -1;
    analogWrite(pino_lampada, brilho_atual);
  }
}
