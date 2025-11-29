#include <Arduino.h>
#include <Wire.h>

// I2C pins (can be overridden via build_flags in platformio.ini)
#ifndef I2C_SDA
  #define I2C_SDA 21
#endif
#ifndef I2C_SCL
  #define I2C_SCL 22
#endif

static const uint8_t CYPD_ADDR = 0x08;

// Your proven-working regs
static const uint16_t REG_PD_STATUS_OLD    = 0x0D10;
static const uint16_t REG_FAULT_STATUS_OLD = 0x0D14;

// Partially-working “plus” regs
static const uint16_t REG_CURRENT_PDO      = 0x1010; // exists
static const uint16_t REG_CURRENT_RDO      = 0x1014; // exists

static const unsigned long POLL_INTERVAL_MS = 1000;
unsigned long lastPoll = 0;
uint8_t lastPdStatus0 = 0xFF;

bool hpiRead(uint16_t reg, uint8_t *buf, size_t len)
{
    Wire.beginTransmission(CYPD_ADDR);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    uint8_t err = Wire.endTransmission(false);

    if (err != 0) {
        Serial.print(F("HPI read: addr 0x"));
        if (reg < 0x100) Serial.print('0');
        Serial.print(reg, HEX);
        Serial.print(F(" write failed, err="));
        Serial.println(err);
        return false;
    }

    size_t readCount = Wire.requestFrom((int)CYPD_ADDR, (int)len);
    if (readCount != len) {
        Serial.print(F("HPI read: addr 0x"));
        if (reg < 0x100) Serial.print('0');
        Serial.print(reg, HEX);
        Serial.print(F(" expected "));
        Serial.print(len);
        Serial.print(F(" bytes, got "));
        Serial.println(readCount);
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        buf[i] = Wire.read();
    }
    return true;
}

void dumpHex(const char *label, const uint8_t *buf, size_t len)
{
    Serial.print(label);
    Serial.print(F(" ["));
    for (size_t i = 0; i < len; i++) {
        if (i > 0) Serial.print(' ');
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
    }
    Serial.println(F("]"));
}

void decodeAndPrintPDO(uint32_t pdo, const char *label)
{
    Serial.print(label);
    Serial.print(F(" = 0x"));
    Serial.println(pdo, HEX);

    // Fixed PDO if bit 31 == 0
    if ((pdo & 0x80000000u) == 0) {
        uint16_t mv = ((pdo >> 10) & 0x3FFu) * 50; // 50 mV units
        uint16_t ma = (pdo & 0x3FFu) * 10;         // 10 mA units

        Serial.print(F("  -> "));
        Serial.print(mv / 1000.0, 3);
        Serial.print(F(" V @ "));
        Serial.print(ma / 1000.0, 3);
        Serial.println(F(" A"));
    } else {
        Serial.println(F("  (Non-fixed PDO type)"));
    }
}

void scanI2C()
{
    Serial.println(F("I2C scan..."));
    for (uint8_t a = 1; a < 127; a++) {
        Wire.beginTransmission(a);
        uint8_t e = Wire.endTransmission();
        if (e == 0) {
            Serial.print(F("  Found device at 0x"));
            if (a < 16) Serial.print('0');
            Serial.println(a, HEX);
        }
    }
    Serial.println();
}

void setup()
{
    Serial.begin(115200);
    delay(800);
    Serial.println();
    Serial.println(F("ESP32 + CYPD3xxx contract debug"));

    Wire.begin(I2C_SDA, I2C_SCL, 400000);
    delay(100);

    scanI2C();
    Serial.println(F("Plug/unplug USB-C and watch PD_STATUS_OLD + CURRENT_PDO..."));
}

void loop()
{
    unsigned long now = millis();
    if (now - lastPoll < POLL_INTERVAL_MS) return;
    lastPoll = now;

    Serial.println(F("---- POLL ----"));

    uint8_t pdOld[4] = {0};
    uint8_t fltOld[4] = {0};

    if (hpiRead(REG_PD_STATUS_OLD, pdOld, 4)) {
        dumpHex("PD_STATUS_OLD (0x0D10)", pdOld, 4);
        uint32_t v = (uint32_t)pdOld[0]
                   | ((uint32_t)pdOld[1] << 8)
                   | ((uint32_t)pdOld[2] << 16)
                   | ((uint32_t)pdOld[3] << 24);
        Serial.print(F("  raw = 0x"));
        Serial.println(v, HEX);
    } else {
        Serial.println(F("PD_STATUS_OLD read failed"));
    }

    if (hpiRead(REG_FAULT_STATUS_OLD, fltOld, 4)) {
        dumpHex("FAULT_STATUS_OLD (0x0D14)", fltOld, 4);
    } else {
        Serial.println(F("FAULT_STATUS_OLD read failed"));
    }

    // Watch for state changes in PD_STATUS_OLD[0]
    if (pdOld[0] != lastPdStatus0) {
        Serial.print(F("PD_STATUS_OLD[0] changed: 0x"));
        if (lastPdStatus0 < 0x10) Serial.print('0');
        Serial.print(lastPdStatus0, HEX);
        Serial.print(F(" -> 0x"));
        if (pdOld[0] < 0x10) Serial.print('0');
        Serial.println(pdOld[0], HEX);
        lastPdStatus0 = pdOld[0];
    }

    // If something is attached / active (pdOld[0] != 0), try to read CURRENT_PDO/RDO
    if (pdOld[0] != 0x00) {
        uint8_t buf[4];

        if (hpiRead(REG_CURRENT_PDO, buf, 4)) {
            dumpHex("CURRENT_PDO (0x1010)", buf, 4);
            uint32_t pdo = (uint32_t)buf[0]
                         | ((uint32_t)buf[1] << 8)
                         | ((uint32_t)buf[2] << 16)
                         | ((uint32_t)buf[3] << 24);
            decodeAndPrintPDO(pdo, "CURRENT_PDO");
        } else {
            Serial.println(F("CURRENT_PDO read failed"));
        }

        if (hpiRead(REG_CURRENT_RDO, buf, 4)) {
            dumpHex("CURRENT_RDO (0x1014)", buf, 4);
            uint32_t rdo = (uint32_t)buf[0]
                         | ((uint32_t)buf[1] << 8)
                         | ((uint32_t)buf[2] << 16)
                         | ((uint32_t)buf[3] << 24);
            Serial.print(F("CURRENT_RDO = 0x"));
            Serial.println(rdo, HEX);
        } else {
            Serial.println(F("CURRENT_RDO read failed"));
        }
    }

    Serial.println();
}
