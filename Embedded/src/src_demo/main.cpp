#include <Arduino.h>
#include <Wire.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

// ============================================================================
// HARDWARE
// ============================================================================
// Various other defs
#define BUTTON_PIN 5

//scent emission

boolean scentemit = false;
const int pwmPin = 32;          // GPIO32
const int pwmChannel = 0;       // LEDC channel (0–15)
const int pwmResolution = 8;    // 8-bit resolution
const int pwmFrequency = 110000; // 110 kHz


// I2C Addresses
static const uint8_t ADDR_LRA = 0x4A; // DA7280 LRA

static const uint8_t ADDR_ERM = 0x48; // DA7281 ERM


// DA728x Registers
static const uint8_t REG_IRQ_EVENT1    = 0x03;
static const uint8_t REG_IRQ_EVENT2    = 0x04;
static const uint8_t REG_IRQ_EVENT3    = 0x05;
static const uint8_t REG_OPERATIONMODE = 0x22;
static const uint8_t REG_OVERRIDE_VAL  = 0x23;

static const uint8_t MODE_INACTIVE = 0x00;
static const uint8_t MODE_DRO      = 0x01;

void scent_function() {
  if (scentemit){
  // Set duty cycle (0–255 for 8-bit)
  ledcWrite(pwmChannel, 128); // 50% duty cycle
  Serial.println("Scent ON");
  } else {
    ledcWrite(pwmChannel, 0); // 0% duty cycle
    Serial.println("Scent OFF");
  }
}

struct regWrite { uint8_t reg; uint8_t val; };

// Tuning table
regWrite lraTuningData[] = {
  {0x03,0x00},{0x04,0x00},{0x05,0x00},{0x07,0x00},{0x08,0x40},
  {0x0A,0x22},{0x0B,0x3C},{0x0C,0x6B},{0x0D,0x6B},{0x0E,0x1F},
  {0x0F,0x00},{0x10,0x98},{0x13,0x18},{0x14,0x11},{0x15,0x03},
  {0x16,0x40},{0x17,0x81},{0x1C,0x0E},{0x1D,0x20},{0x1E,0x03},
  {0x1F,0x20},{0x20,0x43},{0x22,0x00},{0x23,0x36},{0x24,0x08},
  {0x25,0x61},{0x26,0xB4},{0x27,0xEC},{0x28,0x00},{0x29,0x00},
  {0x2A,0x08},{0x2B,0x10},{0x2D,0x80},{0x48,0x1D},{0x49,0x04},
  {0x4C,0x02},{0x5F,0x0E},{0x60,0x9D},{0x62,0x6F},{0x6E,0x00},
  {0x81,0x04},
  {0x83,0x00},{0x84,0x00},{0x85,0x00},{0x86,0x00},{0x87,0x00},
  {0x88,0x00},{0x89,0x00},{0x8A,0x00},{0x8B,0x00},{0x8C,0x00},
  {0x8D,0x00},{0x8E,0x00},{0x8F,0x00},{0x90,0x00},{0x91,0x00},
  {0x92,0x00},{0x93,0x00},{0x94,0x00},{0x95,0x00},{0x96,0x00},
  {0x97,0x00},{0x98,0x00},{0x99,0x00},{0x9A,0x00},{0x9B,0x00},
  {0x9C,0x00},{0x9D,0x00},{0x9E,0x00},{0x9F,0x00},{0xA0,0x00},
  {0xA1,0x00},{0xA2,0x00},{0xA3,0x00},{0xA4,0x00},{0xA5,0x00},
  {0xA6,0x00},{0xA7,0x00},{0xA8,0x00},{0xA9,0x00},{0xAA,0x00},
  {0xAB,0x00},{0xAC,0x00},{0xAD,0x00},{0xAE,0x00},{0xAF,0x00},
  {0xB0,0x00},{0xB1,0x00},{0xB2,0x00},{0xB3,0x00},{0xB4,0x00},
  {0xB5,0x00},{0xB6,0x00},{0xB7,0x00},{0xB8,0x00},{0xB9,0x00},
  {0xBA,0x00},{0xBB,0x00},{0xBC,0x00},{0xBD,0x00},{0xBE,0x00},
  {0xBF,0x00},{0xC0,0x00},{0xC1,0x00},{0xC2,0x00},{0xC3,0x00},
  {0xC4,0x00},{0xC5,0x00},{0xC6,0x00},{0xC7,0x00},{0xC8,0x00},
  {0xC9,0x00},{0xCA,0x00},{0xCB,0x00},{0xCC,0x00},{0xCD,0x00},
  {0xCE,0x00},{0xCF,0x00},{0xD0,0x00},{0xD1,0x00},{0xD2,0x00},
  {0xD3,0x00},{0xD4,0x00},{0xD5,0x00},{0xD6,0x00},{0xD7,0x00},
  {0xD8,0x00},{0xD9,0x00},{0xDA,0x00},{0xDB,0x00},{0xDC,0x00},
  {0xDD,0x00},{0xDE,0x00},{0xDF,0x00},{0xE0,0x00},{0xE1,0x00},
  {0xE2,0x00},{0xE3,0x00},{0xE4,0x00},{0xE5,0x00},{0xE6,0x00},
  {0xE7,0x00}
};

// ============================================================================
// BLUETOOTH AUDIO WITH MUSIC REACTIVE MODE
// ============================================================================
I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

// Music mode state
bool musicModeActive = false;
volatile int32_t bassLevel = 0;
volatile int32_t lastBassLevel = 0;

// Simple low-pass filter for bass extraction
class BassFilter {
  private:
    float alpha = 0.05;  // Low-pass filter coefficient (lower = more bass filtering, less sensitive)
    float filteredValue = 0;
  
  public:
    int32_t process(int16_t sample) {
      filteredValue = alpha * abs(sample) + (1 - alpha) * filteredValue;
      return (int32_t)filteredValue;
    }
};

BassFilter bassFilter;

// Smoothing for haptic output
class HapticSmoother {
  private:
    float smoothFactor = 0.3;  // Higher = more responsive, lower = smoother
    float currentValue = 0;
  
  public:
    int32_t smooth(int32_t newValue) {
      currentValue = smoothFactor * newValue + (1 - smoothFactor) * currentValue;
      return (int32_t)currentValue;
    }
    
    void reset() {
      currentValue = 0;
    }
};

HapticSmoother lraSmoother;
HapticSmoother ermSmoother;

// Audio data callback - processes audio stream in real-time
void audio_data_callback(const uint8_t *data, uint32_t len) {
  // Always pass audio through to speakers first
  i2s.write(data, len);
  
  if (!musicModeActive) return;
  
  // Convert bytes to 16-bit samples
  int16_t *samples = (int16_t*)data;
  int numSamples = len / 2;
  
  // Process samples to extract bass
  int32_t maxBass = 0;
  for (int i = 0; i < numSamples; i += 10) {  // Sample every 10th for performance
    int32_t bass = bassFilter.process(samples[i]);
    if (bass > maxBass) maxBass = bass;
  }
  
  bassLevel = maxBass;
}

// ============================================================================
// BUTTON STATE
// ============================================================================
unsigned long buttonPressStart = 0;
bool buttonPressed = false;

// ============================================================================
// HAPTIC CONTROL
// ============================================================================
static bool writeReg(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return (Wire.endTransmission() == 0);
}

static void programTuningTo(uint8_t addr) {
  for (size_t i = 0; i < sizeof(lraTuningData) / sizeof(lraTuningData[0]); i++) {
    writeReg(addr, lraTuningData[i].reg, lraTuningData[i].val);
  }
}

static void clearIrqWriteOnly(uint8_t addr) {
  writeReg(addr, REG_IRQ_EVENT1, 0xFF);
  writeReg(addr, REG_IRQ_EVENT2, 0xFF);
  writeReg(addr, REG_IRQ_EVENT3, 0xFF);
}

static void setAmp127(uint8_t addr, uint8_t amp) {
  if (amp > 127) amp = 127;
  writeReg(addr, REG_OVERRIDE_VAL, amp);
  writeReg(addr, REG_OPERATIONMODE, (amp == 0) ? MODE_INACTIVE : MODE_DRO);
}

// ============================================================================
// MUSIC MODE
// ============================================================================
void updateMusicHaptics() {
  if (!musicModeActive) return;
  
  // Map bass level (0-32768) to haptic amplitude (0-127)
  // Increased threshold from 8000 to 12000 for less sensitivity
  int32_t mappedLRA = map(bassLevel, 0, 12000, 0, 127);
  int32_t mappedERM = map(bassLevel, 0, 12000, 0, 127);
  
  // Constrain values
  if (mappedLRA < 0) mappedLRA = 0;
  if (mappedLRA > 127) mappedLRA = 127;
  if (mappedERM < 0) mappedERM = 0;
  if (mappedERM > 127) mappedERM = 127;
  
  // Apply smoothing
  mappedLRA = lraSmoother.smooth(mappedLRA);
  mappedERM = ermSmoother.smooth(mappedERM);
  
  // Apply threshold to avoid constant low-level buzz
  if (mappedLRA < 25) mappedLRA = 0;
  if (mappedERM < 25) mappedERM = 0;
  
  // Update haptics
  setAmp127(ADDR_LRA, mappedLRA);
  setAmp127(ADDR_ERM, mappedERM);
  
  lastBassLevel = bassLevel;
}

// ============================================================================
// DEMO MODE
// ============================================================================
void runFullDemo() {
  // Disable music mode during demo
  bool wasMusicActive = musicModeActive;
  musicModeActive = false;
  
  setAmp127(ADDR_LRA, 0);
  setAmp127(ADDR_ERM, 0);
  delay(200);
  
  Serial.println("\n========================================");
  Serial.println("       LRA vs ERM DEMONSTRATION");
  Serial.println("========================================");
  
  // ===== PART 1: Single Click Comparison =====
  scentemit = true;
  Serial.println("\n--- Part 1: Single Click Comparison ---");
  Serial.println("LRA:");
  setAmp127(ADDR_LRA, 100);
  delay(30);
  setAmp127(ADDR_LRA, 0);
  delay(1500);
  
  Serial.println("ERM:");
  setAmp127(ADDR_ERM, 127);
  delay(200);
  setAmp127(ADDR_ERM, 0);
  delay(2000);
  
  
  // ===== PART 2: ERM Spin-Up Showcase =====
  Serial.println("\n--- Part 2: ERM Maximum Strength ---");
  Serial.println("Letting ERM fully spin up...");
  setAmp127(ADDR_ERM, 127);
  delay(1000);
  Serial.println("ERM at full power!");
  delay(1500);
  setAmp127(ADDR_ERM, 0);
  Serial.println("ERM spinning down...");
  delay(2000);
  
  // ===== PART 3: LRA Rapid Fire =====
  scentemit = true;
  Serial.println("\n--- Part 3: LRA Rapid Fire ---");
  for (int i = 0; i < 15; i++) {
    setAmp127(ADDR_LRA, 80);
    delay(30);
    setAmp127(ADDR_LRA, 0);
    delay(60);
  }
  delay(1000);

  
  Serial.println("--- Part 3b: ERM Attempt at Rapid Fire ---");
  for (int i = 0; i < 15; i++) {
    setAmp127(ADDR_ERM, 127);
    delay(150);
    setAmp127(ADDR_ERM, 0);
    delay(200);
  }
  delay(1000);
  
  // ===== PART 4: LRA Heartbeat Pattern =====
 
  Serial.println("\n--- Part 4: LRA Heartbeat ---");
  for (int i = 0; i < 3; i++) {
    setAmp127(ADDR_LRA, 100);
    delay(50);
    setAmp127(ADDR_LRA, 0);
    delay(100);
    setAmp127(ADDR_LRA, 120);
    delay(70);
    setAmp127(ADDR_LRA, 0);
    delay(800);
  }
  delay(1000);

  
  Serial.println("--- Part 4b: ERM Heartbeat Attempt ---");
  for (int i = 0; i < 3; i++) {
    setAmp127(ADDR_ERM, 127);
    delay(200);
    setAmp127(ADDR_ERM, 0);
    delay(300);
    setAmp127(ADDR_ERM, 127);
    delay(200);
    setAmp127(ADDR_ERM, 0);
    delay(800);
  }
  delay(1000);
  
  // ===== PART 5: LRA Amplitude Ramp =====
  Serial.println("\n--- Part 5: LRA Smooth Ramp ---");
  for (int amp = 30; amp <= 120; amp += 10) {
    setAmp127(ADDR_LRA, amp);
    delay(120);
  }
  for (int amp = 120; amp >= 30; amp -= 10) {
    setAmp127(ADDR_LRA, amp);
    delay(120);
  }
  setAmp127(ADDR_LRA, 0);
  delay(1000);
  
  Serial.println("--- Part 5b: ERM Ramp ---");
  for (int amp = 60; amp <= 127; amp += 15) {
    setAmp127(ADDR_ERM, amp);
    delay(300);
  }
  for (int amp = 127; amp >= 60; amp -= 15) {
    setAmp127(ADDR_ERM, amp);
    delay(300);
  }
  setAmp127(ADDR_ERM, 0);
  delay(1000);
  
  // ===== PART 6: LRA Morse Code SOS =====
  Serial.println("\n--- Part 6: LRA Morse Code (SOS) ---");
  // S (...)
  for (int i = 0; i < 3; i++) {
    setAmp127(ADDR_LRA, 90);
    delay(100);
    setAmp127(ADDR_LRA, 0);
    delay(100);
  }
  delay(300);
  // O (---)
  for (int i = 0; i < 3; i++) {
    setAmp127(ADDR_LRA, 90);
    delay(300);
    setAmp127(ADDR_LRA, 0);
    delay(100);
  }
  delay(300);
  // S (...)
  for (int i = 0; i < 3; i++) {
    setAmp127(ADDR_LRA, 90);
    delay(100);
    setAmp127(ADDR_LRA, 0);
    delay(100);
  }
  delay(1500);
  
  Serial.println("--- Part 6b: ERM Morse Code (SOS) ---");
  // S (...)
  for (int i = 0; i < 3; i++) {
    setAmp127(ADDR_ERM, 127);
    delay(200);
    setAmp127(ADDR_ERM, 0);
    delay(200);
  }
  delay(400);
  // O (---)
  for (int i = 0; i < 3; i++) {
    setAmp127(ADDR_ERM, 127);
    delay(500);
    setAmp127(ADDR_ERM, 0);
    delay(200);
  }
  delay(400);
  // S (...)
  for (int i = 0; i < 3; i++) {
    setAmp127(ADDR_ERM, 127);
    delay(200);
    setAmp127(ADDR_ERM, 0);
    delay(200);
  }
  delay(1500);
  
  // ===== PART 7: LRA Typing Simulation =====
  Serial.println("\n--- Part 7: LRA Typing Feel ---");
  for (int i = 0; i < 12; i++) {
    setAmp127(ADDR_LRA, 70);
    delay(25);
    setAmp127(ADDR_LRA, 0);
    delay(random(80, 180));
  }
  delay(1500);
  
  Serial.println("--- Part 7b: ERM Typing Attempt ---");
  for (int i = 0; i < 12; i++) {
    setAmp127(ADDR_ERM, 127);
    delay(100);
    setAmp127(ADDR_ERM, 0);
    delay(random(150, 250));
  }
  delay(1500);
  
  // ===== PART 8: LRA Button Click Pattern =====
  Serial.println("\n--- Part 8: LRA Button Click ---");
  for (int i = 0; i < 3; i++) {
    setAmp127(ADDR_LRA, 110);
    delay(20);
    setAmp127(ADDR_LRA, 0);
    delay(1000);
  }
  delay(1000);
  
  Serial.println("--- Part 8b: ERM Button Click Attempt ---");
  for (int i = 0; i < 3; i++) {
    setAmp127(ADDR_ERM, 127);
    delay(150);
    setAmp127(ADDR_ERM, 0);
    delay(1000);
  }
  delay(1000);
  
  // ===== PART 9: Side-by-Side Final Comparison =====
  Serial.println("\n--- Part 9: Final Side-by-Side Comparison ---");
  Serial.println("LRA:");
  for (int i = 0; i < 5; i++) {
    setAmp127(ADDR_LRA, 90);
    delay(40);
    setAmp127(ADDR_LRA, 0);
    delay(100);
  }
  delay(1000);
  
  Serial.println("ERM:");
  for (int i = 0; i < 5; i++) {
    setAmp127(ADDR_ERM, 127);
    delay(200);
    setAmp127(ADDR_ERM, 0);
    delay(300);
  }
  
  Serial.println("\n========================================");
  Serial.println("         DEMO COMPLETE!");
  Serial.println("========================================");
  Serial.println("\nKEY TAKEAWAYS:");
  Serial.println("• LRA = Instant response, precise control");
  Serial.println("• ERM = Slower, but strong when spun up");
  Serial.println("• LRA = Better for UI feedback");
  Serial.println("• ERM = Better for sustained alerts\n");
  
  // Restore music mode if it was active
  musicModeActive = wasMusicActive;
  if (musicModeActive) {
    Serial.println("Music mode re-enabled");
  }
}

// ============================================================================
// BUTTON HANDLING
// ============================================================================
void handleButton() {
  static bool demoTriggered = false;
  bool currentState = digitalRead(BUTTON_PIN) == LOW;
  
  if (currentState && !buttonPressed) {
    buttonPressed = true;
    buttonPressStart = millis();
    demoTriggered = false;
    Serial.println("Button pressed...");
  } 
  else if (currentState && buttonPressed) {
    unsigned long holdTime = millis() - buttonPressStart;
    
    if (holdTime >= 2000 && !demoTriggered) {
      demoTriggered = true;
      Serial.println("\n→ 2 second hold - Starting demo!");
      delay(200);
      runFullDemo();
    }
  }
  else if (!currentState && buttonPressed) {
    unsigned long holdTime = millis() - buttonPressStart;
    buttonPressed = false;
    
    if (holdTime < 2000) {
      // Toggle music mode
      musicModeActive = !musicModeActive;
      if (musicModeActive) {
        Serial.println("\n♪ MUSIC MODE ENABLED ♪");
        Serial.println("Haptics will pulse with bass!");
        lraSmoother.reset();
        ermSmoother.reset();
      } else {
        Serial.println("\n♪ MUSIC MODE DISABLED");
        setAmp127(ADDR_LRA, 0);
        setAmp127(ADDR_ERM, 0);
        lraSmoother.reset();
        ermSmoother.reset();
      }
    }
  }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(300);

   // scent emission initialization 
  // Configure PWM channel
  ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
  // Attach channel to pin
  ledcAttachPin(pwmPin, pwmChannel);
  ledcWrite(pwmChannel, 128); // 50% duty cycle
  

  Wire.begin(21, 22);
  Wire.setClock(400000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Programming LRA tuning to 0x4A...");
  programTuningTo(ADDR_LRA);
  clearIrqWriteOnly(ADDR_LRA);

  Serial.println("Configuring ERM at 0x48 (minimal config)...");
  clearIrqWriteOnly(ADDR_ERM);
  writeReg(ADDR_ERM, REG_OPERATIONMODE, MODE_DRO);
  delay(100);

  Serial.println("Starting Bluetooth PCM5102A DAC");
  AudioInfo info(44100, 2, 16);
  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.sample_rate = info.sample_rate;
  cfg.channels = info.channels;
  cfg.bits_per_sample = info.bits_per_sample;
  cfg.pin_bck  = 18;
  cfg.pin_ws   = 16;
  cfg.pin_data = 17;

  i2s.end();
  delay(50);
  i2s.begin(cfg);
  delay(50);

  // Set audio callback for music reactive mode
  a2dp_sink.set_stream_reader(audio_data_callback, false);
  a2dp_sink.set_volume(100);
  a2dp_sink.start("AI Audio DAC");
  Serial.println("Bluetooth ready");

  Serial.println("\n=== SYSTEM READY ===");
  Serial.println("Controls:");
  Serial.println("  • Short press = Toggle music mode");
  Serial.println("  • Hold 2 sec  = Run demo");
  Serial.println("\nConnect Bluetooth and play music!");
  Serial.println("Tap button to enable music mode\n");

 
 

  // Initial test
  setAmp127(ADDR_LRA, 127);
  delay(200);
  setAmp127(ADDR_LRA, 0);
  delay(200);
  setAmp127(ADDR_ERM, 127);
  delay(500);
  setAmp127(ADDR_ERM, 0);
  scentemit = true;
  delay(1000);
  scentemit = false;
  ledcWrite(pwmChannel, 0); // 50% duty cycle

}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  handleButton();
  scent_function();
  
  if (musicModeActive) {
    updateMusicHaptics();
  }
  
  delay(10);
}
