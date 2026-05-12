# Relatório - Forno de Fundição Programável com ATmega328P

## 1. Identificação do projeto

Projeto: Forno de Fundição Programável com ATmega328P  
Plataforma: ATmega328P / Arduino UNO em simulação Proteus  
Linguagem: C para AVR usando registradores  
Repositório: <https://github.com/lelepolatto-bit/forno-fundicao-atmega328p>

## 2. Objetivo

O objetivo do projeto é simular o controle de um forno de fundição didático. O sistema permite selecionar perfis de material, controlar uma resistência, ler temperatura por sensor simulado, usar modo manual, acompanhar informações no LCD e na UART, além de parar o processo por emergência.

## 3. Funcionamento geral

Ao iniciar, o LCD mostra a tela de boas-vindas. Depois o usuário seleciona um perfil:

- Alumínio
- Latão
- Personalizado
- Modo manual

Nos perfis Alumínio e Latão, a temperatura alvo já vem definida. No perfil Personalizado, o usuário ajusta a temperatura alvo de 50 em 50°C. Em todos os processos automáticos, o usuário define o tempo de patamar.

Após a confirmação, o sistema passa por pré-aquecimento, aquecimento, patamar, resfriamento natural e finalização. A emergência em D12 interrompe o sistema, desliga a resistência e trava o estado de erro até o usuário liberar o botão e confirmar.

## 4. Perfis disponíveis

| Perfil | Temperatura alvo | Funcionamento |
|---|---:|---|
| Alumínio | 660°C | Automático |
| Latão | 930°C | Automático |
| Personalizado | Ajustável de 50 em 50°C | Automático |
| Modo manual | Potenciômetro A4 | Controle proporcional |

## 5. Sensor de temperatura

O sensor de temperatura é simulado por um potenciômetro no pino A3, canal ADC3. O ADC lê de 0 a 1023, representando uma temperatura física de 0°C a 110°C.

No programa, a temperatura do processo é calculada multiplicando a temperatura física por 10:

```text
sensorTemperatureC = leitura convertida de A3
currentTemperatureC = sensorTemperatureC x 10
```

Exemplo:

```text
Sensor A3 = 66°C
Temperatura simulada do forno = 660°C
```

## 6. Modo manual

No modo manual, o potenciômetro em A4 controla proporcionalmente a resistência em D13. O sensor em A3 continua sendo apenas sensor de temperatura e não altera a potência manual.

Durante o modo manual:

- A4 controla a luminosidade/potência da resistência.
- A3 altera somente a temperatura exibida.
- O LED de estado de aquecimento em A0 fica aceso enquanto o modo manual estiver ativo.
- ENTER sai do modo manual e envia o sistema para resfriamento.

## 7. Patamar

No estado de patamar, o LCD mostra a temperatura do patamar na primeira linha e a contagem de tempo na segunda linha:

```text
Patamar 660°C
3s / 10s
```

Durante o patamar, a saída da resistência em D13 permanece ligada e o LED de aquecimento A0 permanece aceso até o fim do patamar. Ao terminar o tempo configurado, o sistema vai para resfriamento natural.

## 8. Soft starter

No modo automático, a resistência não é liberada diretamente no início. O programa usa uma rampa por software antes de manter a saída da resistência ligada. Isso evita ligar a carga de forma brusca na simulação.

## 9. Máquina de estados

![Mapa de estados](mapa-de-estados.png)

Estados principais:

- INICIO
- CONFIGURACAO
- CONFIRMAR_INICIO
- PRE_AQUECIMENTO
- AQUECIMENTO
- PATAMAR
- RESFRIAMENTO
- FINALIZADO
- MODO_MANUAL
- ERRO

## 10. Desenho do circuito no Proteus

![Desenho do Proteus](circuito-proteus.png)

O circuito usa LCD 16x2, botões físicos, LEDs, potenciômetros para sensor e modo manual, saída para resistência e terminal virtual para UART.

## 11. Pinagem

| Função | Arduino | ATmega328P |
|---|---|---|
| LCD RS | D2 | PD2 |
| LCD E | D3 | PD3 |
| LCD D4 | D4 | PD4 |
| LCD D5 | D5 | PD5 |
| LCD D6 | D6 | PD6 |
| LCD D7 | D7 | PD7 |
| Voltar | D8 | PB0 |
| Enter | D9 | PB1 |
| Menos | D10 | PB2 |
| Mais | D11 | PB3 |
| Emergência | D12 | PB4 |
| Resistência | D13 | PB5 |
| LED Aquecimento | A0 | PC0 |
| LED Finalizado | A1 | PC1 |
| LED Resfriando | A2 | PC2 |
| Sensor de temperatura | A3 | ADC3 / PC3 |
| Potenciômetro manual | A4 | ADC4 / PC4 |
| LED Emergência | A5 | PC5 |

## 12. UART

A UART opera em 9600 baud. A ligação no Proteus deve ser:

- TX do Arduino D1 / PD1 no RXD do Virtual Terminal.
- RX do Arduino D0 / PD0 no TXD do Virtual Terminal.
- GND comum.

Comandos:

| Comando | Função |
|---|---|
| U | Mais |
| D | Menos |
| E | Enter |
| B | Voltar |
| S | Status |
| X | Emergência |

Exemplo de status:

```text
STATUS Modo=AQUECENDO Temp=660°C Alvo=660°C
```

## 13. Arquivos do firmware

| Arquivo | Função |
|---|---|
| firmware/main.c | Loop principal, máquina de estados, ADC, UART, Timer, botões, LEDs e aquecimento |
| firmware/LCDlibrary.c | Biblioteca do LCD 16x2 |
| firmware/PRINCIPALS.h | Includes principais e macros de bit |
| firmware/LCDPRINCIPALS.h | Pinagem e protótipos do LCD |
| firmware/FORNO_CONFIG.h | Defines, enums, struct e mensagens em PROGMEM |

## 14. Testes realizados

Checklist de validação:

1. LCD mostra a tela inicial.
2. Botões D8 a D11 navegam corretamente.
3. Perfis Alumínio, Latão, Personalizado e Manual aparecem.
4. Personalizado ajusta temperatura de 50 em 50°C.
5. Patamar mostra tempo decorrido e tempo total.
6. A3 altera somente a temperatura.
7. A4 controla a resistência no modo manual.
8. LED A0 fica aceso no aquecimento, patamar e modo manual.
9. Emergência em D12 desliga a resistência e aciona erro.
10. UART mostra modo e temperatura.

## 15. Conclusão

O projeto simula um forno de fundição programável usando recursos básicos de sistemas embarcados em C AVR. O firmware trabalha com registradores do ATmega328P, máquina de estados, Timer1, Timer2, ADC, UART, LCD, botões e LEDs, mantendo uma estrutura organizada para simulação e apresentação.

## 16. Links

- GitHub: <https://github.com/lelepolatto-bit/forno-fundicao-atmega328p>
- GitHub Pages: <https://lelepolatto-bit.github.io/forno-fundicao-atmega328p/>
