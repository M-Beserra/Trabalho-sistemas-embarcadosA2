// ---------------------------------------------------------------------------
// Casa Inteligente | 01 | Portão da garagem por controle remoto infravermelho
//
// O que faz: abre e fecha o portão (servo) pelo controle remoto, move o
// portão devagar, fecha sozinho depois de um tempo aberto e volta a abrir se
// a barreira de segurança detectar um obstáculo durante o fechamento.
//
// Entradas : receptor infravermelho (controle remoto) e botão que representa
//            a barreira de segurança (apertado = feixe interrompido).
// Saídas   : servo do portão, LED pisca-alerta e LCD 16x2 I2C.
//
// Teclas do controle do Wokwi:
//   PLAY  -> abre ou fecha (alterna)     +     -> abre
//   POWER -> parada de emergência         -     -> fecha
// ---------------------------------------------------------------------------
#include <IRremote.hpp>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// pinos
const uint8_t pino_ir       = 2;
const uint8_t pino_barreira = 3;   // botão para o GND, pull-up interno
const uint8_t pino_servo    = 9;
const uint8_t pino_alerta   = 7;   // LED pisca-alerta

// códigos NEC das teclas do controle remoto do Wokwi
const uint8_t TECLA_POWER = 162;
const uint8_t TECLA_PLAY  = 168;
const uint8_t TECLA_MAIS  = 2;
const uint8_t TECLA_MENOS = 152;

// ajustes (tempos reduzidos para a simulação)
const int angulo_fechado = 0;
const int angulo_aberto  = 90;
const unsigned long passo_ms          = 40;     // 1 grau a cada 40 ms (~3,6 s de curso)
const unsigned long fechar_sozinho_ms = 15000;  // na prática, 2 minutos
const unsigned long pisca_ms          = 300;

enum class Portao : uint8_t { Fechado, Abrindo, Aberto, Fechando, Parado };

Servo motor;
LiquidCrystal_I2C lcd(0x27, 16, 2);

Portao estado = Portao::Fechado;
int angulo = angulo_fechado;
unsigned long ultimo_passo = 0;
unsigned long aberto_desde = 0;

const char *nome_estado(Portao e) {
  switch (e) {
    case Portao::Fechado:  return "FECHADO";
    case Portao::Abrindo:  return "ABRINDO";
    case Portao::Aberto:   return "ABERTO";
    case Portao::Fechando: return "FECHANDO";
    default:               return "PARADO";
  }
}

void mostrar() {
  char linha[17];
  snprintf(linha, sizeof linha, "Portao: %-8s", nome_estado(estado));
  lcd.setCursor(0, 0);
  lcd.print(linha);
  snprintf(linha, sizeof linha, "Abertura: %3d%% ", (angulo - angulo_fechado) * 100 / (angulo_aberto - angulo_fechado));
  lcd.setCursor(0, 1);
  lcd.print(linha);
}

void mudar(Portao novo) {
  if (novo == estado) return;
  estado = novo;
  if (novo == Portao::Aberto) aberto_desde = millis();
  Serial.print("[PORTAO] ");
  Serial.println(nome_estado(novo));
  mostrar();
}

// Decide o que fazer com cada tecla do controle, conforme o estado atual
void tratar_tecla(uint8_t tecla) {
  if (tecla == TECLA_POWER) {
    if (estado == Portao::Abrindo || estado == Portao::Fechando) mudar(Portao::Parado);
    return;
  }
  bool quer_abrir;
  if (tecla == TECLA_MAIS) quer_abrir = true;
  else if (tecla == TECLA_MENOS) quer_abrir = false;
  else if (tecla == TECLA_PLAY) quer_abrir = !(estado == Portao::Aberto || estado == Portao::Abrindo);
  else return;  // outras teclas não têm função

  if (quer_abrir && angulo < angulo_aberto) mudar(Portao::Abrindo);
  else if (!quer_abrir && angulo > angulo_fechado) mudar(Portao::Fechando);
}

void setup() {
  Serial.begin(9600);
  pinMode(pino_barreira, INPUT_PULLUP);
  pinMode(pino_alerta, OUTPUT);
  IrReceiver.begin(pino_ir);
  motor.attach(pino_servo);
  motor.write(angulo);
  lcd.init();
  lcd.backlight();
  mostrar();
}

void loop() {
  unsigned long agora = millis();

  // 1) controle remoto
  if (IrReceiver.decode()) {
    uint8_t tecla = IrReceiver.decodedIRData.command;
    IrReceiver.resume();
    Serial.print("[PORTAO] Tecla ");
    Serial.println(tecla);
    tratar_tecla(tecla);
  }

  // 2) segurança: obstáculo durante o fechamento faz o portão reabrir
  bool obstaculo = digitalRead(pino_barreira) == LOW;
  if (estado == Portao::Fechando && obstaculo) {
    Serial.println("[PORTAO] Obstaculo! Reabrindo");
    mudar(Portao::Abrindo);
  }

  // 3) fechamento automático (não fecha com obstáculo na frente)
  if (estado == Portao::Aberto && agora - aberto_desde >= fechar_sozinho_ms && !obstaculo) {
    mudar(Portao::Fechando);
  }

  // 4) movimento gradual do servo
  if ((estado == Portao::Abrindo || estado == Portao::Fechando) && agora - ultimo_passo >= passo_ms) {
    ultimo_passo = agora;
    angulo += (estado == Portao::Abrindo) ? 1 : -1;
    motor.write(angulo);
    if (angulo >= angulo_aberto) mudar(Portao::Aberto);
    else if (angulo <= angulo_fechado) mudar(Portao::Fechado);
    else if (angulo % 10 == 0) mostrar();
  }

  // 5) pisca-alerta enquanto o portão se move
  bool movendo = estado == Portao::Abrindo || estado == Portao::Fechando;
  digitalWrite(pino_alerta, movendo && (agora / pisca_ms) % 2 ? HIGH : LOW);
}
