#ifndef FORNO_CONFIG_H_
#define FORNO_CONFIG_H_

#include "PRINCIPALS.h"
#include "LCDPRINCIPALS.h"
#include <avr/pgmspace.h>
#include <stdint.h>

/* Pinos */
#define BUTTON_BACK PB0
#define BUTTON_ENTER PB1
#define BUTTON_DOWN PB2
#define BUTTON_UP PB3
#define SAFETY_BUTTON PB4
#define HEATER_PIN PB5

#define LED_HEATING PC0
#define LED_FINISHED PC1
#define LED_COOLING PC2
#define LED_EMERGENCY PC5

#define SENSOR_ADC_CHANNEL 3U
#define MANUAL_POT_ADC_CHANNEL 4U

/* Eventos */
#define EVENT_NONE 0x00U
#define EVENT_UP 0x01U
#define EVENT_DOWN 0x02U
#define EVENT_ENTER 0x04U
#define EVENT_BACK 0x08U
#define EVENT_STATUS 0x20U
#define EVENT_EMERGENCY 0x40U

/* UART */
#define UART_BAUD 9600UL
#define UART_UBRR_VALUE ((F_CPU / (16UL * UART_BAUD)) - 1UL)

/* Etapas de configuracao */
#define CONFIG_STEP_PROFILE 0U
#define CONFIG_STEP_PROFILE_TEMP 1U
#define CONFIG_STEP_TARGET 2U
#define CONFIG_STEP_SOAKING 3U

/* Temperatura, tempo, histerese e soft starter */
#define PREHEATING_SENSOR_TEMPERATURE_C 30U
#define SENSOR_MAX_TEMPERATURE_C 110U
#define MAX_SENSOR_HEATING_C 80U
#define AMBIENT_TEMPERATURE_C 25U
#define FIXED_HEATING_TIME_S 30U
#define SOAKING_INITIAL_TIME_S 10U
#define SOAKING_STEP_S 10U
#define TARGET_TEMPERATURE_STEP_C 50U
#define HYSTERESIS_C 20U
#define SOFT_START_STEP_PERCENT 10U
#define SOFT_START_STEP_TICKS 5U
#define SOFT_START_WINDOW_TICKS 10U
#define MANUAL_PWM_WINDOW_TICKS 10U

/* Estados */
typedef enum {
    STATE_START,
    STATE_CONFIGURATION,
    STATE_START_CONFIRMATION,
    STATE_PREHEATING,
    STATE_HEATING,
    STATE_SOAKING,
    STATE_COOLING,
    STATE_FINISHED,
    STATE_MANUAL_MODE,
    STATE_ERROR
} SystemState;

/* Perfis */
typedef enum {
    PROFILE_ALUMINUM,
    PROFILE_BRASS,
    PROFILE_CUSTOM,
    PROFILE_MANUAL
} MaterialProfile;

/* Configuracao do processo */
typedef struct {
    MaterialProfile profile;
    uint16_t targetTemperatureC;
    uint16_t heatingTimeS;
    uint16_t soakingTimeS;
} ProcessConfig;

/* Mensagens do LCD */
static const char lcdWelcome0[] PROGMEM = "Bem vindo :)";
static const char lcdWelcome1[] PROGMEM = "Forno Fundicao";
static const char lcdProfile[] PROGMEM = "Selecionar perfil";
static const char lcdTarget[] PROGMEM = "Temp alvo";
static const char lcdSoaking[] PROGMEM = "Tempo patamar";
static const char lcdAluminum[] PROGMEM = "Aluminio";
static const char lcdBrass[] PROGMEM = "Latao";
static const char lcdCustom[] PROGMEM = "Personalizado";
static const char lcdManual[] PROGMEM = "Modo manual";
static const char lcdConfirmStart[] PROGMEM = "Iniciar?";
static const char lcdEnterBack[] PROGMEM = "ENTER / BACK";
static const char lcdPreheating[] PROGMEM = "Pre-aquecendo";
static const char lcdHeatingState[] PROGMEM = "Aquecendo";
static const char lcdCoolingState[] PROGMEM = "Resfriando";
static const char lcdFinished[] PROGMEM = "Finalizado";
static const char lcdPressEnter[] PROGMEM = "ENTER reinicia";
static const char lcdEmergency[] PROGMEM = "EMERGENCIA";
static const char lcdTempLabel[] PROGMEM = "Temp: ";
static const char lcdAlvoLabel[] PROGMEM = "Alvo: ";
static const char lcdPatamarLabel[] PROGMEM = "Patamar ";

/* Mensagens da UART */
static const char uartHelp[] PROGMEM =
    "Comandos: U=MAIS D=MENOS E=ENTER B=VOLTAR S=STATUS X=EMERGENCIA\r\n";
static const char uartStatusTitle[] PROGMEM = "STATUS ";
static const char uartStateLabel[] PROGMEM = "Modo=";
static const char uartTempLabel[] PROGMEM = " Temp=";
static const char uartSensorLabel[] PROGMEM = " Sensor=";
static const char uartTargetLabel[] PROGMEM = " Alvo=";
static const char uartHeaterLabel[] PROGMEM = " Heater=";
static const char uartEmergencyMsg[] PROGMEM = "Emergencia simulada\r\n";
static const char uartNewLine[] PROGMEM = "\r\n";

#endif
