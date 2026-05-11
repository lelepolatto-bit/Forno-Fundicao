#include "PRINCIPALS.h"
#include "LCDPRINCIPALS.h"
#include "FORNO_CONFIG.h"

SystemState currentState = STATE_START;
SystemState previousState = STATE_START;
ProcessConfig currentConfig;
uint8_t configStep = 0U, lcdNeedsUpdate = 1U;
uint16_t sensorTemperatureC = 25U, currentTemperatureC = 250U;
uint8_t emergencyActive = 0U, sensorFailure = 0U, processRunning = 0U;
uint8_t heaterActive = 0U, heaterRequest = 0U, manualPowerPercent = 0U;
volatile uint32_t systemTicks100ms = 0UL, systemSeconds = 0UL;
static uint32_t stateStartSeconds = 0UL, welcomeStartSeconds = 0UL;
static uint32_t softStartLastTick = 0UL, lastLcdProcessTick = 0UL, lastUartStatusSeconds = 0UL;
static uint8_t finishedLedLatched = 0U, emergencyLatched = 0U, softStartDuty = 0U;

ISR(TIMER1_COMPA_vect)
{
    systemTicks100ms++;
    if ((systemTicks100ms % 10U) == 0U) { systemSeconds++; }
}

static uint32_t getSystemSeconds(void)
{
    uint32_t seconds;
    uint8_t sreg = SREG;
    cli();
    seconds = systemSeconds;
    SREG = sreg;
    return seconds;
}

static uint32_t getSystemTicks100ms(void)
{
    uint32_t ticks;
    uint8_t sreg = SREG;
    cli();
    ticks = systemTicks100ms;
    SREG = sreg;
    return ticks;
}

void buttonsInit(void)
{
    clearBit(DDRB, BUTTON_BACK); clearBit(DDRB, BUTTON_ENTER);
    clearBit(DDRB, BUTTON_DOWN); clearBit(DDRB, BUTTON_UP);
    setBit(PORTB, BUTTON_BACK); setBit(PORTB, BUTTON_ENTER);
    setBit(PORTB, BUTTON_DOWN); setBit(PORTB, BUTTON_UP);
}

uint8_t readButtonEvents(void)
{
    static uint8_t previousPressed = 0U;
    uint8_t pressed = 0U, events;
    if (!readBit(PINB, BUTTON_UP)) { pressed |= EVENT_UP; }
    if (!readBit(PINB, BUTTON_DOWN)) { pressed |= EVENT_DOWN; }
    if (!readBit(PINB, BUTTON_ENTER)) { pressed |= EVENT_ENTER; }
    if (!readBit(PINB, BUTTON_BACK)) { pressed |= EVENT_BACK; }
    events = (uint8_t)(pressed & (uint8_t)~previousPressed);
    previousPressed = pressed;
    return events;
}

void safetyButtonInit(void)
{
    clearBit(DDRB, SAFETY_BUTTON);
    setBit(PORTB, SAFETY_BUTTON);
}

void updateSafetyInput(void)
{
    emergencyActive = !readBit(PINB, SAFETY_BUTTON) ? 1U : 0U;
}

void timer1Init(void)
{
    TCCR1A = 0U; TCCR1B = 0U; TCNT1 = 0U; OCR1A = 1562U;
    TCCR1B |= (1U << WGM12);
    TCCR1B |= (1U << CS12) | (1U << CS10);
    TIMSK1 |= (1U << OCIE1A);
}

void adcInit(void)
{
    ADMUX = (uint8_t)(1U << REFS0);
    ADCSRA = (uint8_t)((1U << ADEN) | (1U << ADPS2) | (1U << ADPS1) | (1U << ADPS0));
    DIDR0 |= (uint8_t)((1U << ADC3D) | (1U << ADC4D));
}

uint16_t adcRead(uint8_t channel)
{
    ADMUX = (uint8_t)((ADMUX & 0xF0U) | (channel & 0x0FU));
    setBit(ADCSRA, ADSC);
    while (readBit(ADCSRA, ADSC)) { }
    return ADC;
}

void updateTemperature(void)
{
    uint16_t adcValue = adcRead(SENSOR_ADC_CHANNEL);
    sensorTemperatureC = (uint16_t)(((uint32_t)adcValue * SENSOR_MAX_TEMPERATURE_C) / 1023UL);
    currentTemperatureC = (uint16_t)(sensorTemperatureC * 10U);
}

uint8_t readManualPowerPercent(void)
{
    uint16_t adcValue = adcRead(MANUAL_POT_ADC_CHANNEL);
    return (uint8_t)(((uint32_t)adcValue * 100UL) / 1023UL);
}

void heaterOff(void)
{
    clearBit(PORTB, HEATER_PIN);
    heaterActive = 0U; heaterRequest = 0U; softStartDuty = 0U;
}

void heaterInit(void)
{
    setBit(DDRB, HEATER_PIN);
    heaterOff();
}

void heaterOn(void)
{
    if ((!emergencyLatched) && (!emergencyActive) && (sensorTemperatureC < MAX_SENSOR_HEATING_C)) {
        if (!heaterRequest) { softStartDuty = SOFT_START_STEP_PERCENT; softStartLastTick = getSystemTicks100ms(); }
        heaterRequest = 1U;
    } else {
        heaterOff();
    }
}

void updateHeaterControl(void)
{
    if (emergencyLatched || emergencyActive || sensorFailure || (sensorTemperatureC >= MAX_SENSOR_HEATING_C) ||
        (currentState == STATE_START) || (currentState == STATE_COOLING) ||
        (currentState == STATE_FINISHED) || (currentState == STATE_ERROR)) {
        heaterOff();
        return;
    }
    if ((currentState == STATE_PREHEATING) || (currentState == STATE_HEATING)) {
        if (currentTemperatureC < currentConfig.targetTemperatureC) { heaterOn(); } else { heaterOff(); }
    } else if (currentState == STATE_SOAKING) {
        if (currentTemperatureC < (uint16_t)(currentConfig.targetTemperatureC - HYSTERESIS_C)) { heaterOn(); }
        else if (currentTemperatureC > (uint16_t)(currentConfig.targetTemperatureC + HYSTERESIS_C)) { heaterOff(); }
    } else {
        heaterOff();
    }
}

static void applySoftStarter(void)
{
    uint32_t nowTicks = getSystemTicks100ms();
    uint8_t windowPosition, onTicks;
    if ((!heaterRequest) || emergencyLatched || emergencyActive || sensorFailure ||
        (sensorTemperatureC >= MAX_SENSOR_HEATING_C) || (currentState == STATE_ERROR) ||
        (currentState == STATE_COOLING) || (currentState == STATE_FINISHED) || (currentState == STATE_START)) {
        clearBit(PORTB, HEATER_PIN);
        heaterActive = 0U; heaterRequest = 0U; softStartDuty = 0U;
        return;
    }
    if ((nowTicks - softStartLastTick) >= SOFT_START_STEP_TICKS) {
        softStartLastTick = nowTicks;
        if (softStartDuty < 100U) {
            softStartDuty = (uint8_t)(softStartDuty + SOFT_START_STEP_PERCENT);
            if (softStartDuty > 100U) { softStartDuty = 100U; }
        }
    }
    windowPosition = (uint8_t)(nowTicks % SOFT_START_WINDOW_TICKS);
    onTicks = (uint8_t)((softStartDuty * SOFT_START_WINDOW_TICKS) / 100U);
    if (windowPosition < onTicks) { setBit(PORTB, HEATER_PIN); heaterActive = 1U; }
    else { clearBit(PORTB, HEATER_PIN); heaterActive = 0U; }
}

static void updateManualHeaterControl(void)
{
    uint8_t windowPosition, onTicks;
    manualPowerPercent = readManualPowerPercent();
    if (emergencyLatched || emergencyActive || sensorFailure ||
        (sensorTemperatureC >= MAX_SENSOR_HEATING_C) || (currentState == STATE_ERROR)) {
        heaterOff();
        return;
    }
    heaterRequest = 0U; softStartDuty = 0U;
    if (manualPowerPercent == 0U) {
        clearBit(PORTB, HEATER_PIN);
        heaterActive = 0U;
        return;
    }
    windowPosition = (uint8_t)(getSystemTicks100ms() % MANUAL_PWM_WINDOW_TICKS);
    onTicks = (uint8_t)((manualPowerPercent * MANUAL_PWM_WINDOW_TICKS) / 100U);
    if (windowPosition < onTicks) { setBit(PORTB, HEATER_PIN); heaterActive = 1U; }
    else { clearBit(PORTB, HEATER_PIN); heaterActive = 0U; }
}

void ledsInit(void)
{
    setBit(DDRC, LED_HEATING); setBit(DDRC, LED_FINISHED);
    setBit(DDRC, LED_COOLING); setBit(DDRC, LED_EMERGENCY);
    clearBit(PORTC, LED_HEATING); clearBit(PORTC, LED_FINISHED);
    clearBit(PORTC, LED_COOLING); clearBit(PORTC, LED_EMERGENCY);
}

void updateLeds(void)
{
    if ((currentState == STATE_ERROR) || emergencyLatched || emergencyActive) {
        clearBit(PORTC, LED_HEATING); clearBit(PORTC, LED_FINISHED); clearBit(PORTC, LED_COOLING);
        if (((getSystemTicks100ms() / 5UL) % 2UL) == 0UL) { setBit(PORTC, LED_EMERGENCY); }
        else { clearBit(PORTC, LED_EMERGENCY); }
        return;
    }
    clearBit(PORTC, LED_EMERGENCY);
    if (heaterActive) { setBit(PORTC, LED_HEATING); } else { clearBit(PORTC, LED_HEATING); }
    if (finishedLedLatched) { setBit(PORTC, LED_FINISHED); } else { clearBit(PORTC, LED_FINISHED); }
    if (currentState == STATE_COOLING) { setBit(PORTC, LED_COOLING); } else { clearBit(PORTC, LED_COOLING); }
}

void uartTransmitChar(char data)
{
    while (!readBit(UCSR0A, UDRE0)) { }
    UDR0 = (uint8_t)data;
}

static void uartPrintUInt16(uint16_t value)
{
    char digits[6];
    uint8_t index = 0U;
    if (value == 0U) { uartTransmitChar('0'); return; }
    while ((value > 0U) && (index < sizeof(digits))) {
        digits[index++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    while (index > 0U) { uartTransmitChar(digits[--index]); }
}

void uartPrintTemperature(uint16_t temperature)
{
    uartPrintUInt16(temperature);
    uartTransmitChar((char)0xC2);
    uartTransmitChar((char)0xB0);
    uartTransmitChar('C');
}

void uartPrintFlash(const char *textFlash)
{
    char character = (char)pgm_read_byte(textFlash);
    while (character != '\0') {
        uartTransmitChar(character);
        character = (char)pgm_read_byte(++textFlash);
    }
}

static const char *getModeText(SystemState state)
{
    switch (state) {
    case STATE_START: return PSTR("INICIO");
    case STATE_CONFIGURATION: return PSTR("CONFIG");
    case STATE_START_CONFIRMATION: return PSTR("CONFIRMAR");
    case STATE_PREHEATING: return PSTR("PRE_AQUEC");
    case STATE_HEATING: return PSTR("AQUECENDO");
    case STATE_SOAKING: return PSTR("PATAMAR");
    case STATE_COOLING: return PSTR("RESFRIANDO");
    case STATE_FINISHED: return PSTR("FINALIZADO");
    case STATE_MANUAL_MODE: return PSTR("MANUAL");
    default: return PSTR("EMERGENCIA");
    }
}

void sendStatusUART(void)
{
    uartPrintFlash(uartStatusTitle); uartPrintFlash(uartStateLabel); uartPrintFlash(getModeText(currentState));
    uartPrintFlash(uartTempLabel); uartPrintTemperature(currentTemperatureC);
    if ((currentState == STATE_PREHEATING) || (currentState == STATE_HEATING) ||
        (currentState == STATE_SOAKING) || (currentState == STATE_START_CONFIRMATION) ||
        (currentState == STATE_CONFIGURATION)) {
        uartPrintFlash(uartTargetLabel);
        uartPrintTemperature(currentConfig.targetTemperatureC);
    }
    uartPrintFlash(uartNewLine);
}

void uartInit(void)
{
    UBRR0H = (uint8_t)(UART_UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)UART_UBRR_VALUE;
    UCSR0A = 0U;
    UCSR0B = (uint8_t)((1U << RXEN0) | (1U << TXEN0));
    UCSR0C = (uint8_t)((1U << UCSZ01) | (1U << UCSZ00));
    uartPrintFlash(uartHelp);
}

uint8_t uartAvailable(void)
{
    return readBit(UCSR0A, RXC0) ? 1U : 0U;
}

static const char *getProfileText(MaterialProfile profile)
{
    switch (profile) {
    case PROFILE_ALUMINUM: return lcdAluminum;
    case PROFILE_BRASS: return lcdBrass;
    case PROFILE_MANUAL: return lcdManual;
    default: return lcdCustom;
    }
}

static uint8_t countDigitsUInt16(uint16_t value)
{
    uint8_t count = 1U;
    while (value >= 10U) { value /= 10U; count++; }
    return count;
}

static void lcdPrintUInt16(uint16_t value)
{
    char digits[6];
    uint8_t index = 0U;
    if (value == 0U) { sendData('0', LCD_CHARACTER); return; }
    while ((value > 0U) && (index < sizeof(digits))) {
        digits[index++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    while (index > 0U) { sendData((unsigned char)digits[--index], LCD_CHARACTER); }
}

void lcdPrintTemperature(uint16_t temperature)
{
    lcdPrintUInt16(temperature);
    sendData(0xDFU, LCD_CHARACTER);
    sendData('C', LCD_CHARACTER);
}

static void lcdPrintSpaces(uint8_t count)
{
    while (count > 0U) { sendData(' ', LCD_CHARACTER); count--; }
}

static void lcdFillRemaining(uint8_t used)
{
    if (used < LCD_COLUMNS) { lcdPrintSpaces((uint8_t)(LCD_COLUMNS - used)); }
}

static uint8_t lcdPrintFlashText(const char *textFlash)
{
    uint8_t count = 0U;
    char c = (char)pgm_read_byte(textFlash);
    while (c != '\0') {
        sendData((unsigned char)c, LCD_CHARACTER);
        count++;
        c = (char)pgm_read_byte(++textFlash);
    }
    return count;
}

static void lcdPrintLabelTemperatureLine(const char *labelFlash, uint16_t temperature)
{
    uint8_t used;
    setLine(1U);
    used = lcdPrintFlashText(labelFlash);
    lcdPrintTemperature(temperature);
    lcdFillRemaining((uint8_t)(used + countDigitsUInt16(temperature) + 2U));
}

static void lcdPrintSensorLine(void)
{
    lcdPrintLabelTemperatureLine(lcdTempLabel, currentTemperatureC);
}

void showWelcomeScreen(void)
{
    lcdPrintFlash(0U, lcdWelcome0);
    lcdPrintFlash(1U, lcdWelcome1);
}

void showConfigurationScreen(void)
{
    switch (configStep) {
    case CONFIG_STEP_PROFILE: lcdPrintFlash(0U, lcdProfile); lcdPrintFlash(1U, getProfileText(currentConfig.profile)); break;
    case CONFIG_STEP_PROFILE_TEMP: lcdPrintFlash(0U, getProfileText(currentConfig.profile)); lcdPrintLabelTemperatureLine(lcdAlvoLabel, currentConfig.targetTemperatureC); break;
    case CONFIG_STEP_TARGET: lcdPrintFlash(0U, lcdTarget); lcdPrintLabelTemperatureLine(lcdAlvoLabel, currentConfig.targetTemperatureC); break;
    default:
        lcdPrintFlash(0U, lcdSoaking);
        setLine(1U);
        lcdPrintUInt16(currentConfig.soakingTimeS);
        sendData('s', LCD_CHARACTER);
        lcdFillRemaining((uint8_t)(countDigitsUInt16(currentConfig.soakingTimeS) + 1U));
        break;
    }
}

void showConfirmationScreen(void)
{
    lcdPrintFlash(0U, lcdConfirmStart);
    lcdPrintFlash(1U, lcdEnterBack);
}

void showProcessStatusScreen(void)
{
    if (currentState == STATE_PREHEATING) { lcdPrintFlash(0U, lcdPreheating); }
    else if (currentState == STATE_HEATING) { lcdPrintFlash(0U, lcdHeatingState); }
    else if (currentState == STATE_SOAKING) {
        uint8_t used;
        uint16_t elapsedSeconds = (uint16_t)(getSystemSeconds() - stateStartSeconds);
        if (elapsedSeconds > currentConfig.soakingTimeS) { elapsedSeconds = currentConfig.soakingTimeS; }
        setLine(0U);
        used = lcdPrintFlashText(lcdPatamarLabel);
        lcdPrintTemperature(currentConfig.targetTemperatureC);
        lcdFillRemaining((uint8_t)(used + countDigitsUInt16(currentConfig.targetTemperatureC) + 2U));
        setLine(1U);
        lcdPrintUInt16(elapsedSeconds);
        sendData('s', LCD_CHARACTER);
        sendData(' ', LCD_CHARACTER);
        sendData('/', LCD_CHARACTER);
        sendData(' ', LCD_CHARACTER);
        lcdPrintUInt16(currentConfig.soakingTimeS);
        sendData('s', LCD_CHARACTER);
        lcdFillRemaining((uint8_t)(countDigitsUInt16(elapsedSeconds) +
                                   countDigitsUInt16(currentConfig.soakingTimeS) + 5U));
        return;
    } else {
        lcdPrintFlash(0U, lcdCoolingState);
        lcdPrintSensorLine();
        return;
    }
    setLine(1U);
    lcdPrintTemperature(currentTemperatureC);
    sendData('/', LCD_CHARACTER);
    lcdPrintTemperature(currentConfig.targetTemperatureC);
    lcdFillRemaining((uint8_t)(countDigitsUInt16(currentTemperatureC) +
                               countDigitsUInt16(currentConfig.targetTemperatureC) + 5U));
}

void showManualModeScreen(void)
{
    lcdPrintFlash(0U, lcdManual);
    lcdPrintLabelTemperatureLine(lcdTempLabel, currentTemperatureC);
}

void showFinishedScreen(void)
{
    lcdPrintFlash(0U, lcdFinished);
    lcdPrintFlash(1U, lcdPressEnter);
}

void showErrorScreen(void)
{
    lcdPrintFlash(0U, lcdEmergency);
    lcdPrintFlash(1U, lcdPressEnter);
}

void applyProfile(MaterialProfile profile)
{
    currentConfig.profile = profile;
    currentConfig.heatingTimeS = FIXED_HEATING_TIME_S;
    currentConfig.soakingTimeS = SOAKING_INITIAL_TIME_S;
    switch (profile) {
    case PROFILE_ALUMINUM: currentConfig.targetTemperatureC = 660U; break;
    case PROFILE_BRASS: currentConfig.targetTemperatureC = 930U; break;
    default: currentConfig.targetTemperatureC = 20U; break;
    }
}

uint8_t safetyOkToStart(void)
{
    return (emergencyActive || sensorFailure) ? 0U : 1U;
}

static void changeState(SystemState nextState)
{
    if (currentState == nextState) { return; }
    previousState = currentState;
    currentState = nextState;
    stateStartSeconds = getSystemSeconds();
    lcdNeedsUpdate = 1U;
    if ((nextState == STATE_START) || (nextState == STATE_ERROR) ||
        (nextState == STATE_FINISHED) || (nextState == STATE_COOLING)) { heaterOff(); }
    if ((nextState == STATE_START) || (nextState == STATE_ERROR) || (nextState == STATE_FINISHED)) { processRunning = 0U; }
    if (nextState == STATE_START) { finishedLedLatched = 0U; }
    else if (nextState == STATE_FINISHED) { finishedLedLatched = 1U; }
    else if (nextState == STATE_ERROR) { finishedLedLatched = 0U; }
}

static void goToNaturalCoolingOrFinished(void)
{
    heaterOff();
    if (sensorTemperatureC > AMBIENT_TEMPERATURE_C) { changeState(STATE_COOLING); }
    else { changeState(STATE_FINISHED); }
}

static void handleConfigurationInput(uint8_t events)
{
    if (events & EVENT_BACK) {
        if (configStep == CONFIG_STEP_PROFILE) {
            changeState(STATE_START);
            showWelcomeScreen();
            welcomeStartSeconds = getSystemSeconds();
        } else {
            if ((configStep == CONFIG_STEP_SOAKING) && (currentConfig.profile != PROFILE_CUSTOM)) { configStep = CONFIG_STEP_PROFILE_TEMP; }
            else { configStep--; }
            lcdNeedsUpdate = 1U;
        }
    }
    if (events & EVENT_UP) {
        if (configStep == CONFIG_STEP_PROFILE) { applyProfile((MaterialProfile)((currentConfig.profile + 1U) % 4U)); }
        else if (configStep == CONFIG_STEP_TARGET) { currentConfig.targetTemperatureC += TARGET_TEMPERATURE_STEP_C; }
        else if (configStep == CONFIG_STEP_SOAKING) { currentConfig.soakingTimeS += SOAKING_STEP_S; }
        lcdNeedsUpdate = 1U;
    }
    if (events & EVENT_DOWN) {
        if (configStep == CONFIG_STEP_PROFILE) {
            MaterialProfile nextProfile = (currentConfig.profile == 0U) ? PROFILE_MANUAL : (MaterialProfile)(currentConfig.profile - 1U);
            applyProfile(nextProfile);
        } else if ((configStep == CONFIG_STEP_TARGET) && (currentConfig.targetTemperatureC > 20U)) {
            currentConfig.targetTemperatureC -= TARGET_TEMPERATURE_STEP_C;
            if (currentConfig.targetTemperatureC < 20U) { currentConfig.targetTemperatureC = 20U; }
        } else if ((configStep == CONFIG_STEP_SOAKING) && (currentConfig.soakingTimeS > SOAKING_INITIAL_TIME_S)) {
            currentConfig.soakingTimeS -= SOAKING_STEP_S;
        }
        lcdNeedsUpdate = 1U;
    }
    if (events & EVENT_ENTER) {
        if (configStep == CONFIG_STEP_PROFILE) {
            if (currentConfig.profile == PROFILE_MANUAL) { changeState(STATE_MANUAL_MODE); }
            else { configStep = CONFIG_STEP_PROFILE_TEMP; lcdNeedsUpdate = 1U; }
        } else if (configStep == CONFIG_STEP_PROFILE_TEMP) {
            configStep = (currentConfig.profile == PROFILE_CUSTOM) ? CONFIG_STEP_TARGET : CONFIG_STEP_SOAKING;
            lcdNeedsUpdate = 1U;
        } else if (configStep == CONFIG_STEP_TARGET) {
            configStep = CONFIG_STEP_SOAKING;
            lcdNeedsUpdate = 1U;
        } else {
            changeState(STATE_START_CONFIRMATION);
        }
    }
}

static void handleStateMachine(uint8_t events)
{
    uint32_t nowSeconds = getSystemSeconds(), elapsedSeconds = nowSeconds - stateStartSeconds;
    switch (currentState) {
    case STATE_START:
        processRunning = 0U; heaterOff();
        if ((nowSeconds - welcomeStartSeconds) >= 2UL) { configStep = CONFIG_STEP_PROFILE; applyProfile(PROFILE_ALUMINUM); changeState(STATE_CONFIGURATION); }
        break;
    case STATE_CONFIGURATION: handleConfigurationInput(events); break;
    case STATE_START_CONFIRMATION:
        if ((events & EVENT_ENTER) && safetyOkToStart()) { processRunning = 1U; changeState(STATE_PREHEATING); }
        else if (events & EVENT_BACK) { changeState(STATE_CONFIGURATION); }
        break;
    case STATE_PREHEATING:
        processRunning = 1U; lcdNeedsUpdate = 1U;
        if (sensorTemperatureC >= PREHEATING_SENSOR_TEMPERATURE_C) { changeState(STATE_HEATING); }
        break;
    case STATE_HEATING:
        processRunning = 1U; lcdNeedsUpdate = 1U;
        if ((currentTemperatureC >= currentConfig.targetTemperatureC) || (elapsedSeconds >= currentConfig.heatingTimeS)) { changeState(STATE_SOAKING); }
        break;
    case STATE_SOAKING:
        processRunning = 1U; lcdNeedsUpdate = 1U;
        if (elapsedSeconds >= currentConfig.soakingTimeS) { goToNaturalCoolingOrFinished(); }
        break;
    case STATE_COOLING:
        processRunning = 1U; heaterOff(); lcdNeedsUpdate = 1U;
        if (events & EVENT_ENTER) { changeState(STATE_START); showWelcomeScreen(); welcomeStartSeconds = getSystemSeconds(); }
        else if (sensorTemperatureC <= AMBIENT_TEMPERATURE_C) { changeState(STATE_FINISHED); }
        break;
    case STATE_FINISHED:
        processRunning = 0U; heaterOff();
        if (events & EVENT_ENTER) { changeState(STATE_START); showWelcomeScreen(); welcomeStartSeconds = getSystemSeconds(); }
        break;
    case STATE_MANUAL_MODE:
        processRunning = 1U; lcdNeedsUpdate = 1U;
        if (events & EVENT_ENTER) { heaterOff(); changeState(STATE_COOLING); }
        break;
    case STATE_ERROR: heaterOff(); processRunning = 0U; break;
    default: changeState(STATE_ERROR); break;
    }
}

static void updateLCD(void)
{
    uint32_t nowTicks = getSystemTicks100ms();
    if ((currentState == STATE_PREHEATING) || (currentState == STATE_HEATING) ||
        (currentState == STATE_SOAKING) || (currentState == STATE_COOLING) ||
        (currentState == STATE_MANUAL_MODE)) {
        if ((nowTicks - lastLcdProcessTick) >= 5UL) { lastLcdProcessTick = nowTicks; lcdNeedsUpdate = 1U; }
    }
    if (!lcdNeedsUpdate) { return; }
    switch (currentState) {
    case STATE_START: showWelcomeScreen(); break;
    case STATE_CONFIGURATION: showConfigurationScreen(); break;
    case STATE_START_CONFIRMATION: showConfirmationScreen(); break;
    case STATE_PREHEATING: case STATE_HEATING: case STATE_SOAKING: case STATE_COOLING: showProcessStatusScreen(); break;
    case STATE_FINISHED: showFinishedScreen(); break;
    case STATE_MANUAL_MODE: showManualModeScreen(); break;
    case STATE_ERROR: showErrorScreen(); break;
    default: break;
    }
    lcdNeedsUpdate = 0U;
}

uint8_t uartReadEvents(void)
{
    uint8_t events = EVENT_NONE;
    if (!uartAvailable()) { return events; }
    switch ((char)UDR0) {
    case 'U': case 'u': events = EVENT_UP; break;
    case 'D': case 'd': events = EVENT_DOWN; break;
    case 'E': case 'e': events = EVENT_ENTER; break;
    case 'B': case 'b': events = EVENT_BACK; break;
    case 'S': case 's': sendStatusUART(); events = EVENT_STATUS; break;
    case 'X': case 'x':
        heaterOff();
        processRunning = 0U; emergencyActive = 1U; emergencyLatched = 1U;
        changeState(STATE_ERROR);
        uartPrintFlash(uartEmergencyMsg);
        events = EVENT_EMERGENCY;
        break;
    case 'H': case 'h': uartPrintFlash(uartHelp); break;
    default: break;
    }
    return events;
}

int main(void)
{
    initLCD(); buttonsInit(); safetyButtonInit(); uartInit();
    timer1Init(); adcInit(); heaterInit(); ledsInit();
    sei();
    applyProfile(PROFILE_ALUMINUM);
    showWelcomeScreen();
    welcomeStartSeconds = getSystemSeconds();
    while (1) {
        uint8_t events = readButtonEvents();
        events |= uartReadEvents();
        updateSafetyInput();
        updateTemperature();
        if ((getSystemSeconds() - lastUartStatusSeconds) >= 1U) { lastUartStatusSeconds = getSystemSeconds(); sendStatusUART(); }
        if (emergencyActive) {
            emergencyLatched = 1U;
            heaterOff();
            processRunning = 0U;
            changeState(STATE_ERROR);
        }
        if (currentState == STATE_ERROR) {
            heaterOff();
            if (emergencyLatched && (!emergencyActive) && (events & EVENT_ENTER)) {
                emergencyLatched = 0U;
                changeState(STATE_START);
                showWelcomeScreen();
                welcomeStartSeconds = getSystemSeconds();
            }
        } else {
            handleStateMachine(events);
        }
        if (currentState == STATE_MANUAL_MODE) { updateManualHeaterControl(); }
        else { updateHeaterControl(); applySoftStarter(); }
        updateLeds();
        updateLCD();
    }
}
