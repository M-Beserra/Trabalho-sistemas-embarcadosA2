# Casa Inteligente

Nove automações residenciais com **Arduino Uno**, prontas para o simulador [Wokwi](https://wokwi.com).

Trabalho opcional da A2 de **Sistemas Embarcados** (Prof. Caio Silva Azeredo), Ciência da Computação, Universidade Veiga de Almeida.
Autor: **Matheus Beserra dos Santos** (1240110263).

## Automações

| Pasta | O que faz | Entradas | Saídas |
|---|---|---|---|
| [01-portao-garagem](01-portao-garagem) | Portão por controle remoto, com fechamento automático e proteção contra obstáculo | Receptor IR + controle, botão (barreira) | Servo, LED pisca-alerta, LCD |
| [02-alarme-residencial](02-alarme-residencial) | Alarme com tempo de saída, tempo de entrada e disparo por presença | Chave (porta), PIR, botão | Buzzer, 3 LEDs, LCD |
| [03-luz-corredor](03-luz-corredor) | Luz de vigília à noite e 100 % quando alguém passa | PIR, LDR | LED com PWM |
| [04-cortina-automatica](04-cortina-automatica) | Abre ao amanhecer, fecha à noite, meia-cortina no sol direto | LDR, chave, 2 botões | Servo, LCD |
| [05-irrigacao-jardim](05-irrigacao-jardim) | Rega em ciclos quando o solo está seco e não chove | Potenciômetro (solo), chave (chuva), botão | Relé, LCD |
| [06-ventilador-teto](06-ventilador-teto) | Velocidade pela sensação térmica | DHT22, botão | Display 7 segmentos, LED (motor) |
| [07-comedouro-pet](07-comedouro-pet) | Serve a porção certa pesando o pote | Célula de carga (HX711), botão | Servo, buzzer, LED, LCD |
| [08-campainha](08-campainha) | Melodia, contador de visitas e modo "não perturbe" | Botão, chave | Buzzer, LED, LCD |
| [09-cenas-iluminacao](09-cenas-iluminacao) | Cenas de cor com transição suave | 2 botões, potenciômetro | LED RGB, LCD |

## Executar

Cada pasta é um projeto completo:

- `sketch.ino`: programa comentado;
- `diagram.json`: circuito;
- `libraries.txt`: bibliotecas;
- `sketch.hex` e `sketch.elf`: firmware já compilado;
- `wokwi.toml`: configuração do VS Code.

### No navegador

1. Em [wokwi.com](https://wokwi.com), crie um projeto **Arduino Uno**.
2. Cole `sketch.ino`, `diagram.json` e `libraries.txt` nas abas de mesmo nome.
   - O `libraries.txt` é criado pela seta ao lado das abas.
3. Clique em play.

### No VS Code

1. Instale a extensão **Wokwi Simulator**.
2. Abra a pasta da automação.
3. Pressione `F1` e escolha **Wokwi: Start Simulator**.

### Dicas

- **Automação 1 (controle remoto):**
  - PLAY abre ou fecha;
  - `+` abre e `-` fecha;
  - POWER para o portão.
- **Automação 7 (comedouro):** com a comporta aberta, clique na célula de carga e aumente o peso para simular a ração caindo.
- **Monitor serial:** mostra cada decisão do programa.

## Artigo

[`documentacao/`](documentacao) contém o artigo no formato SBC: o PDF, o LaTeX, as referências e os esquemas dos circuitos.
