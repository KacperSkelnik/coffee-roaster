#include <MAX6675.h>
#include <ModbusRTUSlave.h>

#define SLAVE_ID      1
#define BAUD_RATE     19200
#define NUM_REGS      16

unsigned long lastRead          = 0; // RX, TX
unsigned long ssrStateChanged   = 0;
bool spinning                   = false;

constexpr int SSR_PIN = 7;

ModbusRTUSlave modbusSlave(Serial, -1);

constexpr int thermoBT_GND = 2;
constexpr int thermoBT_VCC = 3;
constexpr int thermoBT_CLK = 4;
constexpr int thermoBT_CS  = 5;
constexpr int thermoBT_DO  = 6;

constexpr int thermoET_GND = 8;
constexpr int thermoET_VCC = 9;
constexpr int thermoET_CLK = 10;
constexpr int thermoET_CS  = 11;
constexpr int thermoET_DO  = 12;

MAX6675 thermocoupleBT(thermoBT_CLK, thermoBT_CS, thermoBT_DO);
MAX6675 thermocoupleET(thermoET_CLK, thermoET_CS, thermoET_DO);

uint16_t holdingRegs[NUM_REGS] = {0};

void setup() {
    pinMode(SSR_PIN, OUTPUT);

    Serial.begin(BAUD_RATE);

    pinMode(thermoBT_VCC, OUTPUT); digitalWrite(thermoBT_VCC, HIGH);
    pinMode(thermoBT_GND, OUTPUT); digitalWrite(thermoBT_GND, LOW);

    pinMode(thermoET_VCC, OUTPUT); digitalWrite(thermoET_VCC, HIGH);
    pinMode(thermoET_GND, OUTPUT); digitalWrite(thermoET_GND, LOW);

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

        const float tempBT_C = thermocoupleBT.readCelsius();
        const float tempET_C = thermocoupleET.readCelsius();

        if (!isnan(tempBT_C)) {
            holdingRegs[0] = static_cast<uint16_t>(tempBT_C * 100);
        } else {
            holdingRegs[0] = 0xFFFF;
        }

        if (!isnan(tempET_C)) {
            holdingRegs[1] = static_cast<uint16_t>(tempET_C * 100);
        } else {
            holdingRegs[1] = 0xFFFF;
        }
    }

    if (now - ssrStateChanged > 250 && !spinning) {
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