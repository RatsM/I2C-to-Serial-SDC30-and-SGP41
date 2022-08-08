#include <Arduino.h>
#include <SensirionI2CSgp41.h>
#include <SensirionI2cScd30.h>
#include <Wire.h>

SensirionI2CSgp41 sgp41;
SensirionI2cScd30 scd30;


static char errorMessage[128];
static int16_t errorSGP41;
static int16_t errorSCD30;
uint16_t conditioning_s = 10;


void setup() {

    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }
    Wire.begin();

    scd30.begin(Wire, SCD30_I2C_ADDR_61);
    sgp41.begin(Wire);




    scd30.stopPeriodicMeasurement();
    scd30.softReset();
    delay(2000);
    uint8_t major = 0;
    uint8_t minor = 0;
    errorSCD30 = scd30.readFirmwareVersion(major, minor);
    if (errorSCD30 != NO_ERROR) {
        Serial.print("Error trying to execute readFirmwareVersion(): ");
        errorToString(errorSCD30, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }
    Serial.print("firmware version major: ");
    Serial.print(major);
    Serial.print("\t");
    Serial.print("minor: ");
    Serial.print(minor);
    Serial.println();
    errorSCD30 = scd30.startPeriodicMeasurement(0);
    if (errorSCD30 != NO_ERROR) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(errorSCD30, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }



    uint16_t serialNumber[3];
    uint8_t serialNumberSize = 3;

    errorSGP41 = sgp41.getSerialNumber(serialNumber, serialNumberSize);

    if (errorSGP41) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(errorSGP41, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("SerialNumber:");
        Serial.print("0x");
        for (size_t i = 0; i < serialNumberSize; i++) {
            uint16_t value = serialNumber[i];
            Serial.print(value < 4096 ? "0" : "");
            Serial.print(value < 256 ? "0" : "");
            Serial.print(value < 16 ? "0" : "");
            Serial.print(value, HEX);
        }
        Serial.println();
    }

    uint16_t testResult;
    errorSGP41 = sgp41.executeSelfTest(testResult);
    if (errorSGP41) {
        Serial.print("Error trying to execute executeSelfTest(): ");
        errorToString(errorSGP41, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (testResult != 0xD400) {
        Serial.print("executeSelfTest failed with error: ");
        Serial.println(testResult);
    }
    
}




void loop() {
    uint16_t defaultRh = 0x8000;
    uint16_t defaultT = 0x6666;
    uint16_t srawVoc = 0;
    uint16_t srawNox = 0;

    float co2Concentration = 0.0;
    float temperature = 0.0;
    float humidity = 0.0;
    delay(1500);
    errorSCD30 = scd30.blockingReadMeasurementData(co2Concentration, temperature,
                                               humidity);

    if (errorSCD30 != NO_ERROR) {
        Serial.print("Error trying to execute blockingReadMeasurementData(): ");
        errorToString(errorSCD30, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }

    if (conditioning_s > 0) {
        // During NOx conditioning (10s) SRAW NOx will remain 0
        errorSGP41 = sgp41.executeConditioning(defaultRh, defaultT, srawVoc);
        conditioning_s--;
    } else {
        // Read Measurement
        errorSGP41 = sgp41.measureRawSignals(defaultRh, defaultT, srawVoc, srawNox);
    }

    if (errorSGP41) {
        Serial.print("Error trying to execute measureRawSignals(): ");
        errorToString(errorSGP41, errorMessage, 256);
        Serial.println(errorMessage);
    }
        
    Serial.print("SRAW_VOC:");
    Serial.print(srawVoc);
    Serial.print("\t");
    Serial.print("SRAW_NOx:");
    Serial.println(srawNox);

    Serial.print("co2Concentration: ");
    Serial.print(co2Concentration);
    Serial.print("\t");
    Serial.print("temperature: ");
    Serial.print(temperature);
    Serial.print("\t");
    Serial.print("humidity: ");
    Serial.print(humidity);
    Serial.println();

    }