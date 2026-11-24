// ---------------------------------------------------------------------------
// Casa Inteligente | 05 | Irrigação do jardim por umidade do solo
//
// O que faz: rega o jardim só quando o solo está seco e não está chovendo.
// A rega é feita em ciclos curtos (rega, pausa para a água infiltrar,
// nova medição), o que evita encharcar o solo e desperdiçar água. Se o solo
// não melhora depois de 4 ciclos, o sistema para e avisa (possível vazamento
// ou sensor com defeito).
//
// Entradas : potenciômetro no lugar do sensor de umidade do solo (o Wokwi não
//            tem esse sensor), chave deslizante "chuva" e botão "Regar agora".
// Saídas   : relé da válvula solenoide (LED azul na carga) e LCD 16x2 I2C.
// ---------------------------------------------------------------------------
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const uint8_t pino_botao   = 2;   // para o GND, pull-up interno
const uint8_t pino_chuva   = 4;   // HIGH = chovendo
const uint8_t pino_valvula = 7;   // relé
const uint8_t pino_solo    = A0;

const int solo_seco    = 30;      // % abaixo do qual começa a regar
const int solo_umido   = 60;      // % em que para de regar
const unsigned long rega_ms     = 8000;    // na prática, 5 min por ciclo
const unsigned long pausa_ms    = 10000;   // na prática, 20 min de infiltração
const unsigned long manual_ms   = 5000;
const uint8_t ciclos_maximos    = 4;

enum Etapa : uint8_t { AGUARDANDO, REGANDO, INFILTRANDO, REGA_MANUAL, FALHA };
const char *const NOMES[] = {"Aguardando", "Regando", "Infiltrando", "Rega manual", "FALHA: verificar"};

LiquidCrystal_I2C lcd(0x27, 16, 2);
Etapa etapa = AGUARDANDO;
unsigned long inicio_etapa = 0;
uint8_t ciclos = 0;
int umidade = 0;
bool chovendo = false;

bool botao_anterior = HIGH;
unsigned long botao_mudou = 0;
bool botao_estavel = HIGH;

bool botao_apertado() {
  bool l = digitalRead(pino_botao);
  if (l != botao_anterior) { botao_anterior = l; botao_mudou = millis(); }
  if (millis() - botao_mudou > 50 && l != botao_estavel) { botao_estavel = l; return l == LOW; }
  return false;
}

void ir_para(Etapa nova) {
  etapa = nova;
  inicio_etapa = millis();
  digitalWrite(pino_valvula, (nova == REGANDO || nova == REGA_MANUAL) ? HIGH : LOW);
  Serial.print("[JARDIM] ");
  Serial.print(NOMES[nova]);
  Serial.print(" | solo ");
  Serial.print(umidade);
  Serial.print(" % | ciclo ");
  Serial.println(ciclos);
}

void setup() {
  Serial.begin(9600);
  pinMode(pino_botao, INPUT_PULLUP);
  pinMode(pino_chuva, INPUT);
  pinMode(pino_valvula, OUTPUT);
  lcd.init();
  lcd.backlight();
  ir_para(AGUARDANDO);
}

void loop() {
  unsigned long t = millis() - inicio_etapa;
  umidade = map(analogRead(pino_solo), 0, 1023, 0, 100);
  chovendo = digitalRead(pino_chuva) == HIGH;
  bool regar_agora = botao_apertado();

  // Chuva interrompe qualquer rega imediatamente
  if (chovendo && (etapa == REGANDO || etapa == INFILTRANDO || etapa == REGA_MANUAL)) {
    ciclos = 0;
    Serial.println("[JARDIM] Chuva detectada: rega cancelada");
    ir_para(AGUARDANDO);
  }

  switch (etapa) {
    case AGUARDANDO:
      if (regar_agora && !chovendo) ir_para(REGA_MANUAL);
      else if (!chovendo && umidade < solo_seco) { ciclos = 1; ir_para(REGANDO); }
      break;
    case REGANDO:
      if (umidade >= solo_umido) { ciclos = 0; ir_para(AGUARDANDO); }
      else if (t >= rega_ms) ir_para(INFILTRANDO);
      break;
    case INFILTRANDO:
      if (t < pausa_ms) break;
      if (umidade >= solo_umido) { ciclos = 0; ir_para(AGUARDANDO); }
      else if (ciclos >= ciclos_maximos) ir_para(FALHA);
      else { ciclos++; ir_para(REGANDO); }
      break;
    case REGA_MANUAL:
      if (t >= manual_ms) ir_para(AGUARDANDO);
      break;
    case FALHA:
      if (regar_agora) { ciclos = 0; ir_para(AGUARDANDO); }   // o botão reconhece a falha
      break;
  }

  static unsigned long tela = 0;
  if (millis() - tela >= 300) {
    tela = millis();
    char l1[17], l2[17];
    snprintf(l1, sizeof l1, "Solo:%3d%% %s", umidade, chovendo ? "CHUVA" : "     ");
    snprintf(l2, sizeof l2, "%-16s", NOMES[etapa]);
    lcd.setCursor(0, 0); lcd.print(l1);
    lcd.setCursor(0, 1); lcd.print(l2);
  }
}
