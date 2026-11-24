// ---------------------------------------------------------------------------
// Casa Inteligente | 04 | Cortina motorizada do quarto
//
// O que faz: no modo automático, abre a cortina quando amanhece, fecha quando
// anoitece e fecha pela metade quando o sol bate direto na janela (para
// evitar calor e ofuscamento). No modo manual, os botões abrem e fecham em
// passos de 25 %.
//
// Entradas : sensor de luminosidade (LDR), chave deslizante Auto/Manual e
//            botões "Abrir" e "Fechar".
// Saídas   : servo da cortina (0 graus = fechada, 180 graus = aberta) e
//            LCD 16x2 I2C.
// ---------------------------------------------------------------------------
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

const uint8_t pino_abrir  = 2;   // botões para o GND, pull-up interno
const uint8_t pino_fechar = 3;
const uint8_t pino_modo   = 4;   // chave: HIGH = automático, LOW = manual
const uint8_t pino_servo  = 6;
const uint8_t pino_ldr    = A0;

// faixas de luz (lux) usadas no modo automático
const float lux_amanheceu   = 50;     // abre acima disso
const float lux_anoiteceu   = 10;     // fecha abaixo disso
const float lux_sol_direto  = 20000;  // meia-cortina acima disso
const float lux_sol_passou  = 12000;  // volta a abrir abaixo disso

const unsigned long passo_servo_ms = 15;   // movimento suave
const unsigned long filtro_ms = 50;

Servo cortina;
LiquidCrystal_I2C lcd(0x27, 16, 2);

int posicao = 0;          // % de abertura atual
int alvo = 0;             // % de abertura desejada
int alvo_automatico = 0;  // última decisão do modo automático
float lux = 0;
unsigned long ultimo_passo = 0, ultima_tela = 0;

// Botões com filtro de repique: guarda o último estado estável de cada um
struct Botao { uint8_t pino; bool estavel; bool bruto; unsigned long mudou; };
Botao b_abrir = {pino_abrir, true, true, 0};
Botao b_fechar = {pino_fechar, true, true, 0};

bool apertou(Botao &b) {
  bool leitura = digitalRead(b.pino);
  if (leitura != b.bruto) { b.bruto = leitura; b.mudou = millis(); }
  if (millis() - b.mudou > filtro_ms && b.estavel != b.bruto) {
    b.estavel = b.bruto;
    return b.estavel == LOW;
  }
  return false;
}

// Curva do LDR do Wokwi: converte a leitura analógica em lux
float ler_lux() {
  int a = constrain(analogRead(pino_ldr), 1, 1022);
  float volts = a * 5.0 / 1024.0;
  float ohms = 2000.0 * volts / (1.0 - volts / 5.0);
  return pow(50e3 * pow(10, 0.7) / ohms, 1.0 / 0.7);
}

// Modo automático: máquina de três situações com histerese
int decidir_automatico(float l, int atual) {
  if (atual == 0) {                       // fechada (noite)
    return l > lux_amanheceu ? 100 : 0;
  }
  if (atual == 100) {                     // aberta (dia)
    if (l < lux_anoiteceu) return 0;
    if (l > lux_sol_direto) return 50;
    return 100;
  }
  // meia-cortina (sol direto)
  if (l < lux_anoiteceu) return 0;
  if (l < lux_sol_passou) return 100;
  return 50;
}

void setup() {
  Serial.begin(9600);
  pinMode(pino_abrir, INPUT_PULLUP);
  pinMode(pino_fechar, INPUT_PULLUP);
  pinMode(pino_modo, INPUT);
  cortina.attach(pino_servo);
  cortina.write(0);
  lcd.init();
  lcd.backlight();
}

void loop() {
  unsigned long agora = millis();
  bool automatico = digitalRead(pino_modo) == HIGH;
  lux = ler_lux();

  if (automatico) {
    int decisao = decidir_automatico(lux, alvo_automatico);
    if (decisao != alvo_automatico) {
      alvo_automatico = decisao;
      Serial.print("[CORTINA] Auto: ");
      Serial.print(decisao);
      Serial.print(" % (luz ");
      Serial.print((long)lux);
      Serial.println(" lux)");
    }
    alvo = alvo_automatico;
  }
  // Os botões funcionam só no manual; leitura feita sempre para não acumular toques
  bool abrir = apertou(b_abrir), fechar = apertou(b_fechar);
  if (!automatico) {
    if (abrir)  alvo = min(100, alvo + 25);
    if (fechar) alvo = max(0, alvo - 25);
    if (abrir || fechar) { Serial.print("[CORTINA] Manual: "); Serial.print(alvo); Serial.println(" %"); }
  }

  // Movimento suave: 1 % a cada 15 ms
  if (posicao != alvo && agora - ultimo_passo >= passo_servo_ms) {
    ultimo_passo = agora;
    posicao += (alvo > posicao) ? 1 : -1;
    cortina.write(map(posicao, 0, 100, 0, 180));
  }

  if (agora - ultima_tela >= 400) {
    ultima_tela = agora;
    char l1[17], l2[17];
    snprintf(l1, sizeof l1, "Cortina: %3d%%  ", posicao);
    snprintf(l2, sizeof l2, "%-6s %5ld lx ", automatico ? "AUTO" : "MANUAL", (long)(lux > 99999 ? 99999 : lux));
    lcd.setCursor(0, 0); lcd.print(l1);
    lcd.setCursor(0, 1); lcd.print(l2);
  }
}
