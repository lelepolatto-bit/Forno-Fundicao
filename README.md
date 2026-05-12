# Forno de Fundição Programável com ATmega328P

   Projeto de um forno de fundição programável com ATmega328P. O sistema permite selecionar perfis de fundição para Alumínio e Latão, configurar um modo Personalizado, operar em Modo Manual com potenciômetro, controlar uma resistência com soft starter, monitorar temperatura via ADC, exibir dados no LCD 16x2 e no terminal serial, além de contar com LEDs de sinalização e botão de emergência. O funcionamento é organizado por máquina de estados e simulado no Proteus.

## Objetivo

   O objetivo do projeto é simular o controle de um forno de fundição em ambiente didático. O sistema permite selecionar perfis de material, configurar tempo de patamar, ler temperatura por ADC, usar modo manual por potenciômetro, acompanhar o estado pelo LCD/UART e interromper o processo por emergência física.

## Funcionalidades

- Seleção de perfis de operação: Alumínio, Latão, Personalizado e Modo Manual
- Perfis pré-configurados com temperaturas reais de fundição
- Modo Personalizado com ajuste da temperatura alvo e tempo de patamar
- Modo Manual com controle proporcional da resistência por potenciômetro
- Leitura de temperatura por ADC, com sensor conectado ao pino A3
- Exibição de informações em display LCD 16x2
- Controle por botões físicos: Mais, Menos, Enter, Voltar e Emergência
- Comunicação UART com envio do modo atual, temperatura, sensor e estado da resistência
- Temporização com Timer1, sem uso de delays longos
- Máquina de estados para controlar as etapas do processo
- Acionamento da resistência com soft starter por software
- Resfriamento natural após o tempo de patamar ou saída do modo manual
- Sistema de emergência com travamento até confirmação por Enter
- LEDs de sinalização para aquecimento, finalizado, resfriamento e emergência
- Simulação completa no Proteus

## Perfis disponíveis

| Perfil | Temperatura alvo | Funcionamento |
|---|---:|---|
| Alumínio | 660°C | Automático |
| Latão | 930°C | Automático |
| Personalizado | Ajustável de 50 em 50°C | Automático |
| Modo manual | Potenciômetro A4 | Controle proporcional |

## Funcionamento geral

O funcionamento do forno é organizado por uma máquina de estados. Ao iniciar, o sistema exibe uma mensagem de boas-vindas no LCD e, em seguida, entra na etapa de configuração.

A sequência principal do processo é:

   1. O sistema inicia e exibe a tela inicial.
   2. O usuário seleciona o modo de operação: Alumínio, Latão, Personalizado ou Modo Manual.
   3. Nos perfis Alumínio e Latão, a temperatura alvo já é pré-configurada.
   4. No modo Personalizado, o usuário ajusta a temperatura alvo de 50 em 50°C.
   5. O usuário define o tempo de patamar, ajustado de 10 em 10 segundos.
   6. O sistema solicita a confirmação de início.
   7. Após a confirmação, o forno entra em pré-aquecimento.
   8. Em seguida, inicia o aquecimento até atingir a temperatura alvo ou finalizar o tempo fixo de aquecimento.
   9. Ao entrar no patamar, o sistema mantém a temperatura próxima da temperatura alvo durante o tempo configurado.
   10. Após o patamar, a resistência é desligada e o sistema entra em resfriamento natural.
   11. Quando o resfriamento termina, o processo é finalizado e o LED de finalização acende.
   12. A emergência pode interromper o processo a qualquer momento.

## Modo manual

O modo manual usa um potenciômetro no pino A4. Esse potenciômetro controla proporcionalmente a resistência em D13. O sensor em A3 continua sendo lido durante o modo manual, e a temperatura exibida é o valor do sensor multiplicado por 10.

Ao apertar ENTER no modo manual, o sistema desliga a resistência e entra no resfriamento. No estado de resfriamento, ENTER volta ao início.

## Modo manual

No Modo Manual, a resistência é controlada proporcionalmente por um potenciômetro ligado ao pino A4. Quanto maior o valor ajustado no potenciômetro, maior o tempo de acionamento da resistência em D13.

Durante esse modo, o sensor de temperatura no pino A3 continua sendo lido em tempo real. A temperatura exibida no LCD e enviada pela UART corresponde ao valor do sensor multiplicado por 10.

A tela do LCD mostra:

Modo manual
Temp: xx°C

Ao pressionar ENTER no Modo Manual, a resistência é desligada e o sistema entra no estado de resfriamento. Durante o resfriamento, ao pressionar ENTER novamente, o sistema retorna ao início.

## Sensor de temperatura

A leitura de temperatura é feita por um sensor simulado conectado ao pino A3, utilizando o canal ADC3 do ATmega328P.

O ADC lê valores de 0 a 1023. No código, essa leitura é convertida para uma escala física de 0°C a 110°C.

A temperatura usada no processo é calculada da seguinte forma:

Temperatura do processo = temperatura do sensor × 10

Exemplo:

Sensor = 66°C
Temperatura do processo = 660°C

Essa multiplicação permite simular temperaturas maiores de fundição usando uma faixa menor de leitura no Proteus.

## Soft starter

A resistência de aquecimento é acionada pelo pino D13, mas ela não liga diretamente em potência máxima.

O código utiliza um soft starter por software, baseado no Timer1. O acionamento começa com pulsos menores e aumenta gradualmente até atingir o acionamento total. Isso simula uma partida mais suave da resistência e evita uma ativação brusca do aquecimento.

## Máquina de estados

![Mapa de estados](images/mapa-de-estados.png)

O firmware é controlado por uma máquina de estados, que organiza cada etapa do funcionamento do forno.

Estados usados no projeto:

INICIO: exibe a mensagem inicial e prepara o sistema.
CONFIGURACAO: permite selecionar o perfil e ajustar os parâmetros.
CONFIRMAR_INICIO: aguarda a confirmação para iniciar o processo.
PRE_AQUECIMENTO: realiza o aquecimento inicial.
AQUECIMENTO: aquece até a temperatura alvo ou até finalizar o tempo fixo.
PATAMAR: mantém a temperatura próxima do valor configurado.
RESFRIAMENTO: desliga a resistência e aguarda o resfriamento natural.
FINALIZADO: indica que o processo terminou.
MODO_MANUAL: permite controlar a resistência pelo potenciômetro em A4.
ERRO: estado acionado pela emergência, desligando imediatamente a resistência.

## Pinagem

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
| Sensor temperatura | A3 | ADC3 / PC3 |
| Potenciômetro manual | A4 | ADC4 / PC4 |
| LED Emergência | A5 | PC5 |

## LEDs

| LED | Pino | Função |
|---|---|---|
| Aquecimento | A0 | Acende quando a resistência está ativa |
| Finalizado | A1 | Acende quando o processo termina |
| Resfriando | A2 | Acende no resfriamento |
| Emergência | A5 | Pisca em emergência |

## Botões

| Botão | Pino | Função |
|---|---|---|
| Voltar | D8 | Voltar/cancelar |
| Enter | D9 | Confirmar |
| Menos | D10 | Diminuir |
| Mais | D11 | Aumentar |
| Emergência | D12 | Parar o sistema |

## UART

A UART usa 9600 baud, 8 bits, sem paridade e 1 stop bit. Para usar o Virtual Terminal no Proteus:

| Comando | Função |
|---|---|
| U | Mais |
| D | Menos |
| E | Enter |
| B | Voltar |
| S | Status |
| X | Emergência |

Exemplo de saída atual:

```text
STATUS Modo=AQUECENDO Temp=660°C Alvo=660°C
```

## Simulação no Proteus

<img width="669" height="462" alt="image" src="https://github.com/user-attachments/assets/554cb732-5cd3-4950-a495-03206e695fe3" />

O projeto foi preparado para simulação no Proteus usando ATmega328P/Arduino UNO, LCD 16x2, botões, LEDs, potenciômetros, terminal virtual e circuito de acionamento da resistência.


## Como compilar

1. Abra o Microchip Studio / Atmel Studio.
2. Crie ou abra um projeto para ATmega328P.
3. Adicione em Source Files:
   - `firmware/main.c`
   - `firmware/LCDlibrary.c`
4. Adicione em Header Files:
   - `firmware/PRINCIPALS.h`
   - `firmware/LCDPRINCIPALS.h`
   - `firmware/FORNO_CONFIG.h`
5. Compile o projeto.
6. Carregue o `.hex` no Proteus.

Também existe o script `build.ps1` para compilar com `avr-gcc`, caso o toolchain esteja instalado.

## Como testar

1. Abrir a simulação no Proteus.
2. Iniciar o circuito.
3. Ver LCD inicial.
4. Selecionar perfil.
5. Ajustar tempo de patamar.
6. Confirmar início.
7. Observar aquecimento.
8. Ver status na UART.
9. Testar modo manual com A4.
10. Testar emergência em D12.
11. Ver LEDs.


