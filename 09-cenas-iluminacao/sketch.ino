// ---------------------------------------------------------------------------
// Casa Inteligente | 09 | Cenas de iluminação da sala com LED RGB
//
// O que faz: um botão percorre cenas prontas (Leitura, Jantar, Cinema e
// Festa), com transição suave de 1 s entre as cores. O potenciômetro regula
// o brilho geral. Na cena Festa, as cores giram sozinhas. O outro botão
// apaga a sala com esmaecimento.
//
// Entradas : botão "Cena", botão "Desligar" e potenciômetro de brilho.
// Saídas   : LED RGB (fita de LED da sanca, catodo comum) e LCD 16x2 I2C.
// ---------------------------------------------------------------------------
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const uint8_t pino_cena     = 2;   // botões para o GND, pull-up interno
const uint8_t pino_desligar = 3;
const uint8_t pino_brilho   = A0;
const uint8_t pino_r = 9, pino_g = 10, pino_b = 11;   // PWM

struct Cor { uint8_t r, g, b; };
struct Cena { const char *nome; Cor cor; };

const Cena CENAS[] = {
  {"Leitura", {255, 244, 229}},   // branco neutro, luz para ler
  {"Jantar",  {255, 147, 41}},    // âmbar quente
  {"Cinema",  {20, 30, 120}},     // azul baixo, sem reflexo na tela
  {"Festa",   {255, 0, 0}},       // ponto de partida do ciclo de cores
};
const uint8_t TOTAL_CENAS = sizeof(CENAS) / sizeof(CENAS[0]);
const uint8_t CENA_FESTA = 3;
const unsigned long transicao_ms = 1000;

LiquidCrystal_I2C lcd(0x27, 16, 2);

int cena = -1;                  // -1 = sala apagada
Cor cor_de = {0, 0, 0}, cor_para = {0, 0, 0}, cor_agora = {0, 0, 0};
unsigned long inicio_transicao = 0;
uint8_t brilho = 255;
uint16_t matiz = 0;             // usado no ciclo da cena Festa (0 a 359 graus)

struct Botao { uint8_t pino; bool antes; unsigned long t; };
Botao b_cena = {pino_cena, HIGH, 0}, b_desligar = {pino_desligar, HIGH, 0};

bool apertou(Botao &b) {
  bool l = digitalRead(b.pino);
  if (l != b.antes && millis() - b.t > 50) { b.t = millis(); b.antes = l; return l == LOW; }
  return false;
}

// Converte matiz (0-359) em cor RGB com saturação e valor máximos
Cor roda_de_cores(uint16_t h) {
  uint8_t setor = h / 60, f = (h % 60) * 255 / 60;
  switch (setor) {
    case 0:  return {255, f, 0};
    case 1:  return {(uint8_t)(255 - f), 255, 0};
    case 2:  return {0, 255, f};
    case 3:  return {0, (uint8_t)(255 - f), 255};
    case 4:  return {f, 0, 255};
    default: return {255, 0, (uint8_t)(255 - f)};
  }
}

uint8_t misturar(uint8_t a, uint8_t b, unsigned long t) {
  return a + ((long)b - a) * (long)t / (long)transicao_ms;
}

void iniciar_transicao(Cor destino) {
  cor_de = cor_agora;
  cor_para = destino;
  inicio_transicao = millis();
}

void atualizar_lcd() {
  char l1[17], l2[17];
  snprintf(l1, sizeof l1, "Cena: %-10s", cena < 0 ? "Apagada" : CENAS[cena].nome);
  snprintf(l2, sizeof l2, "Brilho: %3d%%    ", brilho * 100 / 255);
  lcd.setCursor(0, 0); lcd.print(l1);
  lcd.setCursor(0, 1); lcd.print(l2);
}

void setup() {
  Serial.begin(9600);
  pinMode(pino_cena, INPUT_PULLUP);
  pinMode(pino_desligar, INPUT_PULLUP);
  pinMode(pino_r, OUTPUT);
  pinMode(pino_g, OUTPUT);
  pinMode(pino_b, OUTPUT);
  lcd.init();
  lcd.backlight();
  atualizar_lcd();
}

void loop() {
  unsigned long agora = millis();

  if (apertou(b_cena)) {
    cena = (cena + 1) % TOTAL_CENAS;
    matiz = 0;
    iniciar_transicao(CENAS[cena].cor);
    Serial.print("[SALA] Cena: ");
    Serial.println(CENAS[cena].nome);
    atualizar_lcd();
  }
  if (apertou(b_desligar) && cena >= 0) {
    cena = -1;
    iniciar_transicao({0, 0, 0});
    Serial.println("[SALA] Apagando");
    atualizar_lcd();
  }

  // Brilho geral pelo potenciômetro (só atualiza o LCD quando muda bastante)
  uint8_t novo_brilho = analogRead(pino_brilho) / 4;
  if (abs((int)novo_brilho - (int)brilho) > 3) { brilho = novo_brilho; atualizar_lcd(); }

  // Transição linear entre a cor de partida e a de destino
  unsigned long t = agora - inicio_transicao;
  if (t >= transicao_ms) {
    cor_agora = cor_para;
    // Na cena Festa, terminada a transição, a cor gira continuamente
    if (cena == CENA_FESTA) {
      static unsigned long ultimo_giro = 0;
      if (agora - ultimo_giro >= 20) { ultimo_giro = agora; matiz = (matiz + 1) % 360; cor_agora = cor_para = roda_de_cores(matiz); }
    }
  } else {
    cor_agora = {misturar(cor_de.r, cor_para.r, t), misturar(cor_de.g, cor_para.g, t), misturar(cor_de.b, cor_para.b, t)};
  }

  analogWrite(pino_r, cor_agora.r * brilho / 255);
  analogWrite(pino_g, cor_agora.g * brilho / 255);
  analogWrite(pino_b, cor_agora.b * brilho / 255);
}
