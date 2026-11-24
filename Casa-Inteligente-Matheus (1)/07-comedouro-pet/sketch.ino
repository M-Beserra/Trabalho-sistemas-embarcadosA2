// ---------------------------------------------------------------------------
// Casa Inteligente | 07 | Comedouro automático do pet com balança
//
// O que faz: nos horários programados (ou pelo botão), libera ração até o
// pote atingir a porção de 100 g. Se o pote ainda tem ração suficiente, não
// serve de novo (evita excesso). Se a ração não cai em 8 s, o reservatório
// provavelmente acabou: o sistema fecha a comporta e avisa.
//
// Entradas : célula de carga com módulo HX711 (peso do pote) e botão
//            "Servir agora".
// Saídas   : servo da comporta, buzzer, LED "reservatório vazio" e LCD 16x2.
//
// No Wokwi: durante a liberação, aumente o peso da célula de carga (clique
// nela) para simular a ração caindo no pote.
// ---------------------------------------------------------------------------
#include <HX711.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const uint8_t pino_hx_dt  = A1;
const uint8_t pino_hx_sck = A0;
const uint8_t pino_botao  = 2;   // para o GND, pull-up interno
const uint8_t pino_servo  = 6;
const uint8_t pino_buzzer = 8;
const uint8_t pino_led_vazio = 9;

const float escala = 0.42;                     // célula de 5 kg do Wokwi: leitura bruta 0-2100 = 0-5000 g
const float porcao_g = 100;                    // porção por refeição
const float minimo_para_pular_g = 60;          // com 60 g ou mais no pote, não serve
const unsigned long intervalo_refeicao_ms = 30000;  // na prática, a cada 8 horas
const unsigned long tempo_maximo_ms = 8000;    // sem atingir a porção: reservatório vazio
const int comporta_fechada = 0, comporta_aberta = 70;

enum Situacao : uint8_t { ESPERANDO, SERVINDO, VAZIO };

HX711 balanca;
Servo comporta;
LiquidCrystal_I2C lcd(0x27, 16, 2);

Situacao situacao = ESPERANDO;
float peso = 0;
unsigned long proxima_refeicao = 0;
unsigned long inicio_servico = 0;
unsigned long ultimo_bipe = 0;
uint8_t refeicoes_servidas = 0;
bool botao_era = HIGH;
unsigned long botao_t = 0;

bool pediu_agora() {
  bool b = digitalRead(pino_botao);
  bool borda = false;
  if (b != botao_era && millis() - botao_t > 50) { botao_t = millis(); botao_era = b; borda = (b == LOW); }
  return borda;
}

void tela() {
  char l1[17], l2[17];
  snprintf(l1, sizeof l1, "Pote: %4d g    ", (int)peso);
  if (situacao == SERVINDO) snprintf(l2, sizeof l2, "Servindo...     ");
  else if (situacao == VAZIO) snprintf(l2, sizeof l2, "RACAO ACABOU!   ");
  else {
    long falta = ((long)proxima_refeicao - (long)millis()) / 1000;
    snprintf(l2, sizeof l2, "Prox. em %3lds  ", falta < 0 ? 0L : (falta > 999 ? 999L : falta));
  }
  lcd.setCursor(0, 0); lcd.print(l1);
  lcd.setCursor(0, 1); lcd.print(l2);
}

// Início de uma refeição: decide se precisa servir
void refeicao(const char *origem) {
  proxima_refeicao = millis() + intervalo_refeicao_ms;
  Serial.print("[COMEDOURO] Refeicao (");
  Serial.print(origem);
  Serial.print("), pote com ");
  Serial.print((int)peso);
  Serial.println(" g");
  if (peso >= minimo_para_pular_g) {
    Serial.println("[COMEDOURO] Ainda ha racao no pote: nao serve");
    tone(pino_buzzer, 600, 120);
    return;
  }
  situacao = SERVINDO;
  inicio_servico = millis();
  comporta.write(comporta_aberta);
}

void terminar(Situacao nova) {
  comporta.write(comporta_fechada);
  situacao = nova;
  if (nova == ESPERANDO) {
    refeicoes_servidas++;
    tone(pino_buzzer, 1800, 150);   // "a comida está servida"
    Serial.print("[COMEDOURO] Porcao servida. Total hoje: ");
    Serial.println(refeicoes_servidas);
  } else {
    Serial.println("[COMEDOURO] Racao nao caiu: reservatorio vazio");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(pino_botao, INPUT_PULLUP);
  pinMode(pino_buzzer, OUTPUT);
  pinMode(pino_led_vazio, OUTPUT);
  balanca.begin(pino_hx_dt, pino_hx_sck);
  balanca.set_scale(escala);
  comporta.attach(pino_servo);
  comporta.write(comporta_fechada);
  lcd.init();
  lcd.backlight();
  proxima_refeicao = millis() + intervalo_refeicao_ms;
}

void loop() {
  unsigned long agora = millis();
  if (balanca.is_ready()) peso = max(0.0f, balanca.get_units(1));
  bool pedido = pediu_agora();

  switch (situacao) {
    case ESPERANDO:
      if (pedido) refeicao("botao");
      else if ((long)(agora - proxima_refeicao) >= 0) refeicao("horario");
      break;
    case SERVINDO:
      if (peso >= porcao_g) terminar(ESPERANDO);
      else if (agora - inicio_servico >= tempo_maximo_ms) terminar(VAZIO);
      break;
    case VAZIO:
      // Bipe a cada 3 s até alguém reabastecer e apertar o botão
      if (agora - ultimo_bipe >= 3000) { ultimo_bipe = agora; tone(pino_buzzer, 900, 300); }
      if (pedido) {
        situacao = ESPERANDO;
        Serial.println("[COMEDOURO] Reabastecido: tentando servir de novo");
        refeicao("reabastecido");
      }
      break;
  }
  digitalWrite(pino_led_vazio, situacao == VAZIO ? HIGH : LOW);

  static unsigned long ultima_tela = 0;
  if (agora - ultima_tela >= 300) { ultima_tela = agora; tela(); }
}
