#ifndef CJ125_H
#define CJ125_H

#include <Arduino.h>

#define CJ125_MISO   MISO
#define CJ125_MOSI   MOSI
#define CJ125_SCLK   SCK
#define CJ125_SS     SS

#define UA_ANALOG_READ_PIN    35     
#define UB_ANALOG_READ_PIN    34
#define UR_ANALOG_READ_PIN    32

#define MAXIMUM_LAMBDA_ADC_VALUE    791
#define MINIMUM_LAMBDA_ADC_VALUE    39
#define MAXIMUM_LAMBDA_VALUE        10.119
#define MINIMUM_LAMBDA_VALUE        0.750

#define MAXIMUM_OXYGEN_ADC_VALUE    854
#define MINIMUM_OXYGEN_ADC_VALUE    307

#define MINIMUM_BATTERY_ADC_VALUE   550

typedef enum _cj125_responses_: uint16_t {
    STATUS_STARTUP,
    STATUS_OK   = 0x28FF,
    STATUS_NO_POWER = 0x2855,
    STATUS_NO_SENSOR = 0x287f,
    STATUS_0 = 0x2888,
    STATUS_1 = 0x2889
}CJ125_RESPONSE;

typedef enum _cj125_requests_: uint16_t {
    STARTUP,
    IDENTIFY = 0x4800,
    DIAGNOSTIC = 0x7800,
    FIRST_INIT = 0x6C00,
    SECOND_INIT = 0x7E00,
    FIRST_INIT_CALIBRATION = 0x569D,
    FIRST_INIT_MODE_8V = 0x5688,
    FIRST_INIT_MODE_17V = 0x5689
}CJ125_REQUEST;

typedef struct _adc_read_ {
    uint16_t UB;
    uint16_t UA;
    uint16_t UR;
}ADC_READ;

extern ADC_READ optimalCjConfig;

void cj125PinInitialize();

void cj125SpiInitalize();

void cj125Startup();

void cj125Calibration();

CJ125_RESPONSE cj125SendRequest(CJ125_REQUEST data);

float translateLambdaValue(uint16_t data);

float translateOxygenValue(uint16_t data);

ADC_READ readCjValues();

boolean isBatteryAlright();

void displayValues();

void setHeaterPWM(uint16_t PWM);

#endif