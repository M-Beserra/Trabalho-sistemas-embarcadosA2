// ---------------------------------------------------------------------------
// Casa Inteligente | 08 | Campainha com modo "não perturbe"
//
// O que faz: ao apertar a campainha, toca uma melodia curta e registra a
// visita. Toques repetidos em menos de 4 s contam como a mesma visita (a
// pessoa insistindo). Com o modo "não perturbe" ligado, a campainha fica
// muda: só o LED pisca e a visita fica guardada como perdida. Ao desligar
// esse modo, o display mostra quantas visitas foram perdidas.
//
// Entradas : botão da campainha e chave deslizante "Não perturbe".
// Saídas   : buzzer (melodia), LED de aviso e LCD 16x2 I2C.
// ---------------------------------------------------------------------------
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const uint8_t pino_campainha = 2;   // para o GND, pull-up interno
const uint8_t pino_silencio  = 4;   // chave: HIGH = não perturbe ligado
const uint8_t pino_buzzer    = 8;
const uint8_t pino_led       = 9;

const unsigned long mesma_visita_ms = 4000;
const unsigned long led_aviso_ms    = 5000;

// melodia "ding-dong-ding" (frequência em Hz, duração em ms)
struct Nota { unsigned int hz; unsigned int ms; };
const Nota MELODIA[] = {{1319, 220}, {1047, 220}, {1568, 400}};
const uint8_t TOTAL_NOTAS = sizeof(MELODIA) / sizeof(MELODIA[0]);

LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned int visitas = 0, perdidas = 0;
unsigned long ultima_visita = 0;
bool ja_teve_visita = false;
int nota_atual = -1;                 // -1 = melodia parada
unsigned long inicio_nota = 0;
unsigned long led_ate = 0;
bool silencio_antes = false;
bool botao_antes = HIGH;
unsigned long botao_t = 0;

void escrever(const char *l1, const char *l2) {
  char b[17];
  snprintf(b, sizeof b, "%-16s", l1); lcd.setCursor(0, 0); lcd.print(b);
  snprintf(b, sizeof b, "%-16s", l2); lcd.setCursor(0, 1); lcd.print(b);
}

void painel_normal() {
  char l1[17], l2[17];
  snprintf(l1, sizeof l1, "Visitas: %u", visitas);
  snprintf(l2, sizeof l2, "%s", digitalRead(pino_silencio) ? "Nao perturbe" : "Campainha ativa");
  escrever(l1, l2);
}

// Toca a melodia sem travar o programa: uma nota por vez, pelo relógio
void atualizar_melodia() {
  if (nota_atual < 0) return;
  if (millis() - inicio_nota < MELODIA[nota_atual].ms) return;
  nota_atual++;
  if (nota_atual >= TOTAL_NOTAS) { nota_atual = -1; noTone(pino_buzzer); return; }
  inicio_nota = millis();
  tone(pino_buzzer, MELODIA[nota_atual].hz);
}

void tocar() {
  nota_atual = 0;
  inicio_nota = millis();
  tone(pino_buzzer, MELODIA[0].hz);
}

void registrar_toque(bool silencio) {
  unsigned long agora = millis();
  bool insistencia = ja_teve_visita && agora - ultima_visita < mesma_visita_ms;
  ultima_visita = agora;
  ja_teve_visita = true;
  led_ate = agora + led_aviso_ms;
  if (insistencia) {
    Serial.println("[CAMPAINHA] Toque repetido: mesma visita");
    return;                                      // não toca de novo nem conta
  }
  visitas++;
  if (silencio) {
    perdidas++;
    Serial.println("[CAMPAINHA] Visita em modo nao perturbe (so LED)");
    escrever("Visita (mudo)", "");
  } else {
    Serial.println("[CAMPAINHA] Ding-dong!");
    tocar();
    escrever("Ding-dong!", "Alguem na porta");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(pino_campainha, INPUT_PULLUP);
  pinMode(pino_silencio, INPUT);
  pinMode(pino_buzzer, OUTPUT);
  pinMode(pino_led, OUTPUT);
  lcd.init();
  lcd.backlight();
  silencio_antes = digitalRead(pino_silencio);
  painel_normal();
}

void loop() {
  unsigned long agora = millis();
  bool silencio = digitalRead(pino_silencio) == HIGH;

  bool b = digitalRead(pino_campainha);
  if (b != botao_antes && agora - botao_t > 50) {
    botao_t = agora;
    botao_antes = b;
    if (b == LOW) registrar_toque(silencio);
  }

  // Ao sair do "não perturbe", avisa as visitas perdidas
  if (silencio != silencio_antes) {
    silencio_antes = silencio;
    if (!silencio && perdidas > 0) {
      char l2[17];
      snprintf(l2, sizeof l2, "%u perdida(s)", perdidas);
      escrever("Enquanto mudo:", l2);
      Serial.print("[CAMPAINHA] Visitas perdidas: ");
      Serial.println(perdidas);
      perdidas = 0;
      led_ate = agora + led_aviso_ms;
    } else {
      painel_normal();
    }
  }

  atualizar_melodia();

  // LED pisca enquanto houver aviso pendente
  bool aviso = (long)(led_ate - agora) > 0;
  digitalWrite(pino_led, aviso && (agora / 250) % 2 ? HIGH : LOW);
  static bool aviso_antes = false;
  if (aviso_antes && !aviso) painel_normal();    // aviso terminou: volta ao painel
  aviso_antes = aviso;
}
