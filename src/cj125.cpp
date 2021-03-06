#include "cj125.h"
#include "SPI.h"
#include "logger.h"
#include "analog_write.h"
#include "conversion.h"

SPIClass * vspi = NULL;
CJ125_RESPONSE responseStatus = STATUS_STARTUP;

static const uint32_t spiClk = 2000000; // 2 MHz
static uint32_t programTime = 0;
static uint32_t actionTime = 0;
static uint32_t pwmIntervalTime = 0;
uint16_t heaterPWM = 0;
float powerSupply = 0; 

#define HEATER_PWM_PIN 20 //////////////TODO

ADC_READ optimalCjConfig = {0,0,0};
ADC_READ cjReadValues = {0,0,0};


void cj125PinInitialize(){
  pinMode(UR_ANALOG_READ_PIN, INPUT);
  pinMode(UB_ANALOG_READ_PIN, INPUT);
  pinMode(UA_ANALOG_READ_PIN, INPUT);
  analogRead(UA_ANALOG_READ_PIN);
  pinMode(CJ125_SS, OUTPUT); 
}

void cj125SpiInitalize() 
{
    SPI.begin(CJ125_SCLK, CJ125_MISO, CJ125_MOSI, CJ125_SS);
    SPI.setDataMode(SPI_MODE1);
    SPI.setClockDivider(SPI_CLOCK_DIV16);
    digitalWrite(CJ125_SS, HIGH);
}

void cj125Startup()
{
  while(true)
  {
    programTime = millis();

    if(programTime - actionTime > 1000)
    {
      actionTime = programTime;

      responseStatus = cj125SendRequest(DIAGNOSTIC);
      cjReadValues.UB = analogRead(UB_ANALOG_READ_PIN);

      if(responseStatus == STATUS_OK && cjReadValues.UB < MINIMUM_BATTERY_ADC_VALUE)
      {
        logInfo("Device is ready");
        return;
      } 
      else 
      {
        if(responseStatus != STATUS_OK)
        {
          logError(responseStatus);
        }

        if(cjReadValues.UB < MINIMUM_BATTERY_ADC_VALUE)
        {
          logError("Battery is too low");
          
        }
      }
    }
  }

  //TODO
  //zapalenie diody działania
}



void cj125Calibration(){
  logInfo("Strting calibration");
  cj125SendRequest(FIRST_INIT_CALIBRATION);

  delay(500);

  optimalCjConfig.UA = analogRead(UA_ANALOG_READ_PIN);
  optimalCjConfig.UR = analogRead(UA_ANALOG_READ_PIN);

  cjReadValues.UA = optimalCjConfig.UA;

  cj125SendRequest(FIRST_INIT_MODE_17V);

  Serial.print("UA_Optimal (λ = 1.00): ");
  Serial.print(optimalCjConfig.UA);
  Serial.print(" (λ = ");
  Serial.print(translateLambdaValue(optimalCjConfig.UA), BIN);
  Serial.print(")\n\r");
  Serial.print("UR_Optimal: ");
  Serial.print(optimalCjConfig.UR);
  Serial.print("\n\r");
}

void condensationPhase(){
  
  powerSupply = (float)cjReadValues.UB / 1023 * 3.3 * 3; // te wartosci trzeba posprawdzac zgodnie z dzielnikami
  heaterPWM = ( 2 / powerSupply) * 255; // te wartosci trzeba posprawdzac zgodnie z dzielnikami i zobaczyc jak działa analog write
  setHeaterPWM(heaterPWM);

  //TODO przerobić z delay na time + dodać miganie diody

  logInfo("Condensation phase please wait");
  delay(5000);
  logInfo("Done");
}

void rampUpPhase() {
  float UHeater = 8.5;
  logInfo("Ramp Up phase please wait");
  while(UHeater < 13.0) {

    heaterPWM = (UHeater / powerSupply) * 255;

    if(heaterPWM > 255) heaterPWM = 255;

    setHeaterPWM(heaterPWM);

    delay(1000); // TODO przerobić delay + dodać miganie diody w innej czestotliwosci
    UHeater += 0.4;
    Serial.print(".");
  }
  logInfo("Done");
}

void optimalHeatingPhase(){

  logInfo("Optimal heating phase");
  while(analogRead(UR_ANALOG_READ_PIN) > optimalCjConfig.UR)
  {
    delay(200); //TODO przerobić delay dodać miganie diody w jeszcze innej czestotliwosci
    Serial.print(".");
  }
  logInfo("Done");

  setHeaterPWM(0);
}

boolean isAdcLambdaValueInRange(uint16_t data) {
  return data >= MINIMUM_LAMBDA_ADC_VALUE && data <= MAXIMUM_LAMBDA_ADC_VALUE;
}

float translateLambdaValue(uint16_t data){

  float result;

  if(isAdcLambdaValueInRange(data))
  {
    result = pgm_read_float_near(LAMBDA_CONVERSION_VALUE + (data - MINIMUM_LAMBDA_ADC_VALUE));
  } 
  else if (data > MAXIMUM_LAMBDA_ADC_VALUE) 
  {
    result = MAXIMUM_LAMBDA_VALUE;
  }
  else
  {
    result = MINIMUM_LAMBDA_VALUE;
  }

  return result;
}

boolean isAdcOxygenValueInRange(uint16_t data){
  return data >= MINIMUM_OXYGEN_ADC_VALUE && data <= MAXIMUM_OXYGEN_ADC_VALUE;
}

float translateOxygenValue(uint16_t data){
  float result = 0;

  if(isAdcOxygenValueInRange(data))
  {
    result = pgm_read_float_near(OXYGEN_CONVERSION_VALUE + (data - MINIMUM_OXYGEN_ADC_VALUE));
  }

  return result;
}

CJ125_RESPONSE cj125SendRequest_v2(CJ125_REQUEST data) {

  uint16_t response;
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(CJ125_SS, LOW); //pull SS slow to prep other end for transfer
  response = vspi->transfer16(data);  
  digitalWrite(CJ125_SS, HIGH); //pull ss high to signify end of data transfer
  vspi->endTransaction();

  return static_cast<CJ125_RESPONSE>(response);
}

CJ125_RESPONSE cj125SendRequest(CJ125_REQUEST data)
{
  CJ125_RESPONSE Response;

  digitalWrite(CJ125_SS, LOW);

  logInfo(data);
  Response = static_cast<CJ125_RESPONSE>(SPI.transfer16(data));
  logInfo(Response);

  digitalWrite(CJ125_SS, HIGH);

  return Response;
  
}

ADC_READ readCjValues()
{
  responseStatus = cj125SendRequest(DIAGNOSTIC);

  cjReadValues.UA = analogRead(UA_ANALOG_READ_PIN);
  cjReadValues.UB = analogRead(UB_ANALOG_READ_PIN);
  cjReadValues.UR = analogRead(UR_ANALOG_READ_PIN);

  return cjReadValues;
}

boolean isBatteryAlright()
{
  if(cjReadValues.UB > MINIMUM_BATTERY_ADC_VALUE) 
  {
    return true;
  }
  else
  {
    logInfo("Battery is low");
  }
}

void displayValues()
{
  const float LAMBDA_VALUE = translateLambdaValue(cjReadValues.UA);
  const float OXYGEN_CONTENT = translateOxygenValue(cjReadValues.UA);

    //Update analog output.
    //UpdateAnalogOutput();
      
    //Display information if no errors is reported.
    if (responseStatus == STATUS_OK) {
      
      //Assemble data.
      String txString = "Measuring, CJ125: 0x";
      txString += String(responseStatus, HEX);
      txString += ", UA_ADC: ";
      txString += String(cjReadValues.UA, DEC);
      txString += ", UR_ADC: ";
      txString += String(cjReadValues.UR, DEC);
      txString += ", UB_ADC: ";
      txString += String(cjReadValues.UB, DEC);

      //Display lambda value unless out of range.
      if (isAdcLambdaValueInRange(cjReadValues.UA)) {
          txString += ", Lambda: ";
          txString += String(LAMBDA_VALUE, 2);
      } else {
          txString += ", Lambda: -";
      }

      //Display oxygen unless out of range.
      if (isAdcOxygenValueInRange(cjReadValues.UA)) {
        txString += ", Oxygen: ";
        txString += String(OXYGEN_CONTENT, 2);
        txString += "%";
      } else {
        txString += ", Oxygen: -";
      }
      
      //Output string
      logInfo(txString);
  }
}

void setHeaterPWM(uint16_t PWM)
{
    analogWrite(HEATER_PWM_PIN, PWM);
}



