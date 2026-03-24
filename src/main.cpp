#include <MAX6675.h>
#include <ModbusRTUSlave.h>

#define SLAVE_ID      1
#define BAUD_RATE     19200
#define NUM_REGS      16

unsigned long lastRead          = 0; // RX, TX
unsigned long ssrStateChanged   = 0;
bool spinning                   = false;

ModbusRTUSlave modbusSlave(Serial, -1);

constexpr int thermoGND = 2;
constexpr int thermoVCC = 3;
constexpr int thermoCLK = 4;
constexpr int thermoCS  = 5;
constexpr int thermoDO  = 6;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

uint16_t holdingRegs[NUM_REGS] = {0};

constexpr int SSR_PIN = 7;

void setup() {
    Serial.begin(BAUD_RATE);

    pinMode(thermoVCC, OUTPUT); digitalWrite(thermoVCC, HIGH);
    pinMode(thermoGND, OUTPUT); digitalWrite(thermoGND, LOW);

    pinMode(SSR_PIN, OUTPUT);

    delay(500);

    // Register the holding registers array once, here in setup
    modbusSlave.configureHoldingRegisters(holdingRegs, NUM_REGS);
    modbusSlave.begin(SLAVE_ID, BAUD_RATE);
}

void loop() {
    modbusSlave.poll();

    const unsigned long now = millis();
    if (now - lastRead > 200) {
        lastRead = now;

        const float tempC = thermocouple.readCelsius();

        if (!isnan(tempC)) {
            holdingRegs[0] = static_cast<uint16_t>(tempC * 100);
        } else {
            holdingRegs[0] = 0xFFFF;
        }
    }

    if (now - ssrStateChanged > 1000 && !spinning) {
        ssrStateChanged = now;
        spinning = true;

        digitalWrite(SSR_PIN, HIGH);
    }

    if (now - ssrStateChanged > 1000 && spinning) {
        ssrStateChanged = now;
        spinning = false;

        digitalWrite(SSR_PIN, LOW);
    }
}