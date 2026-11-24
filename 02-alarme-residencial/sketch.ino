// ---------------------------------------------------------------------------
// Casa Inteligente | 02 | Alarme residencial com tempo de saída e de entrada
//
// O que faz: protege a casa com um sensor de porta e um sensor de presença.
// Ao armar, o morador tem 10 s para sair. Se a porta abrir com o alarme
// armado, há 10 s para desarmar antes da sirene. Movimento dentro de casa com
// o alarme armado dispara na hora.
//
// Entradas : chave deslizante (sensor magnético da porta), sensor PIR e
//            botão de comando (toque curto arma; segurar 2 s desarma).
// Saídas   : buzzer (sirene), LEDs verde, amarelo e vermelho e LCD 16x2 I2C.
// ---------------------------------------------------------------------------
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const uint8_t pino_porta    = 2;   // HIGH = porta aberta
const uint8_t pino_pir      = 3;
const uint8_t pino_botao    = 4;   // para o GND, pull-up interno
const uint8_t pino_sirene   = 8;
const uint8_t led_verde     = 9;
const uint8_t led_amarelo   = 10;
const uint8_t led_vermelho  = 11;

const unsigned long tempo_saida_ms    = 10000;
const unsigned long tempo_entrada_ms  = 10000;
const unsigned long sirene_max_ms     = 30000;  // limite de tempo da sirene
const unsigned long segurar_ms        = 2000;   // toque longo para desarmar
const unsigned long filtro_botao_ms   = 50;

enum class Modo : uint8_t { Desarmado, Saida, Armado, Entrada, Disparado };

LiquidCrystal_I2C lcd(0x27, 16, 2);
Modo modo = Modo::Desarmado;
unsigned long inicio_modo = 0;
const char *motivo = "";
long ultimo_segundo = -1;   // última contagem mostrada no LCD
int fase_sirene = -1;       // tom atual da sirene (-1 = nenhum)

// --- leitura do botão: distingue toque curto e toque longo ---------------
struct LeituraBotao { bool curto; bool longo; };
bool botao_preso = false, longo_ja_avisado = false;
unsigned long botao_desde = 0, ultima_borda = 0;

LeituraBotao ler_botao() {
  LeituraBotao r = {false, false};
  bool apertado = digitalRead(pino_botao) == LOW;
  unsigned long agora = millis();
  if (apertado != botao_preso && agora - ultima_borda > filtro_botao_ms) {
    ultima_borda = agora;
    botao_preso = apertado;
    if (apertado) { botao_desde = agora; longo_ja_avisado = false; }
    else if (!longo_ja_avisado) r.curto = true;          // soltou antes de 2 s
  }
  if (botao_preso && !longo_ja_avisado && agora - botao_desde >= segurar_ms) {
    longo_ja_avisado = true;
    r.longo = true;                                       // segurou 2 s
  }
  return r;
}

void leds(bool verde, bool amarelo, bool vermelho) {
  digitalWrite(led_verde, verde);
  digitalWrite(led_amarelo, amarelo);
  digitalWrite(led_vermelho, vermelho);
}

void escrever(const char *l1, const char *l2) {
  char b[17];
  snprintf(b, sizeof b, "%-16s", l1); lcd.setCursor(0, 0); lcd.print(b);
  snprintf(b, sizeof b, "%-16s", l2); lcd.setCursor(0, 1); lcd.print(b);
}

void entrar(Modo novo) {
  modo = novo;
  inicio_modo = millis();
  ultimo_segundo = -1;
  fase_sirene = -1;
  noTone(pino_sirene);
  switch (novo) {
    case Modo::Desarmado: leds(true, false, false); escrever("Alarme", "DESARMADO"); Serial.println("[ALARME] Desarmado"); break;
    case Modo::Saida:     Serial.println("[ALARME] Armando: 10 s para sair"); break;
    case Modo::Armado:    leds(false, false, true); escrever("Alarme", "ARMADO"); Serial.println("[ALARME] Armado"); break;
    case Modo::Entrada:   Serial.println("[ALARME] Porta aberta: 10 s para desarmar"); break;
    case Modo::Disparado: escrever("DISPARADO!", motivo); Serial.print("[ALARME] DISPARO: "); Serial.println(motivo); break;
  }
}

// Mostra a contagem regressiva e bipa uma vez por segundo
void contagem(const char *titulo, unsigned long total, unsigned long decorrido, bool rapido) {
  long falta = (total - decorrido + 999) / 1000;
  if (falta != ultimo_segundo) {
    ultimo_segundo = falta;
    char l2[17];
    snprintf(l2, sizeof l2, "%ld s", falta);
    escrever(titulo, l2);
    tone(pino_sirene, rapido ? 2400 : 1600, rapido ? 250 : 80);
  }
  leds(false, (decorrido / 250) % 2, false);  // amarelo piscando
}

void setup() {
  Serial.begin(9600);
  pinMode(pino_porta, INPUT);
  pinMode(pino_pir, INPUT);
  pinMode(pino_botao, INPUT_PULLUP);
  pinMode(pino_sirene, OUTPUT);
  pinMode(led_verde, OUTPUT);
  pinMode(led_amarelo, OUTPUT);
  pinMode(led_vermelho, OUTPUT);
  lcd.init();
  lcd.backlight();
  entrar(Modo::Desarmado);
}

void loop() {
  unsigned long decorrido = millis() - inicio_modo;
  LeituraBotao botao = ler_botao();
  bool porta_aberta = digitalRead(pino_porta) == HIGH;
  bool movimento = digitalRead(pino_pir) == HIGH;

  // segurar 2 s desarma em qualquer situação
  if (botao.longo && modo != Modo::Desarmado) { entrar(Modo::Desarmado); return; }

  switch (modo) {
    case Modo::Desarmado:
      if (botao.curto) entrar(Modo::Saida);
      break;

    case Modo::Saida:
      if (decorrido >= tempo_saida_ms) {
        if (porta_aberta) { motivo = "Porta aberta"; entrar(Modo::Disparado); }  // saiu sem fechar
        else entrar(Modo::Armado);
      } else contagem("Saia e feche", tempo_saida_ms, decorrido, false);
      break;

    case Modo::Armado:
      if (movimento) { motivo = "Presenca interna"; entrar(Modo::Disparado); }
      else if (porta_aberta) entrar(Modo::Entrada);
      break;

    case Modo::Entrada:
      if (decorrido >= tempo_entrada_ms) { motivo = "Invasao porta"; entrar(Modo::Disparado); }
      else contagem("Desarme: segure", tempo_entrada_ms, decorrido, true);
      break;

    case Modo::Disparado: {
      int pisca = (decorrido / 300) % 2;
      leds(false, false, pisca);
      if (decorrido < sirene_max_ms) {
        if (pisca != fase_sirene) { fase_sirene = pisca; tone(pino_sirene, pisca ? 2200 : 1500); }  // sirene de dois tons
      } else if (fase_sirene != 2) {
        fase_sirene = 2;
        noTone(pino_sirene);   // sirene para, mas o LED segue avisando
      }
      break;
    }
  }
}
