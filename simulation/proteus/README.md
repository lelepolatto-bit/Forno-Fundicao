# Simulação no Proteus

Arquivo principal:

```text
forno-fundicao.pdsprj
```

## Como testar

1. Abra o arquivo `.pdsprj` no Proteus.
2. Compile o firmware no Microchip Studio / Atmel Studio ou rode `build.ps1`.
3. Carregue o arquivo `.hex` gerado no microcontrolador da simulação.
4. Inicie a simulação.
5. Verifique o LCD, botões, LEDs, terminal virtual, sensor A3 e potenciômetro A4.

## UART

Use o Virtual Terminal em 9600 baud:

- TX do Arduino D1 / PD1 -> RXD do terminal
- RX do Arduino D0 / PD0 -> TXD do terminal
- GND comum
