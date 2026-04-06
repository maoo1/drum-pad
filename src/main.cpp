#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

void handleHit(int index);
void sendMessage(const char* msg);
int fastTouchRead(int pin);
void printDebugInfo();

const char* apSSID = "DRUM_PAD_WIFI";
const char* apPassword = "drumpad123";
const char* laptopIP = "192.168.4.2";
const int laptopPort = 5005;
WiFiUDP udp;

// --- PIN DEFINITIONS ---
const int PIN_CTR_A = 13;
const int PIN_CTR_B = 15; 
const int PIN_RIM_A = 12;
const int PIN_RIM_B = 33;    
const int PIN_MODE  = 27;
int pins[5] = {PIN_CTR_A, PIN_CTR_B, PIN_RIM_A, PIN_RIM_B, PIN_MODE};

// --- TUNED THRESHOLDS BASED ON YOUR LOGS ---
// A HIT is detected if the DROP is HIGHER than these:
/*
const int hitThresholds[5] = {
  22,  // Pad A  (Resting drop is ~44)
  8,  // Pad B  (Resting drop is ~60)
  25,  // Pad RA (Resting drop is ~40)
  20,  // Pad RB (Resting drop is ~0)
  25   // Mode   (Estimated)
};

// A Pad RE-ARMS (resets) only if the DROP falls BELOW these:
const int releaseThresholds[5] = {
  12,  // Pad A  (Must get back near 44)
  5,  // Pad B  (Must get back near 60)
  12,  // Pad RA (Must get back near 40)
  20,  // Pad RB (Must get back near 0)
  10   // Mode
};
*/

const int touchSensitivity[5] = {
  15, // Pad A
  5,  // Pad B (Very low because your raw signal is weak)
  15, // RA
  5, // RB (Your strongest pad)
  15  // Mode
};

const int holdThreshold = 10; // For Mute logic
const int cooldown = 35;      

float baselines[5] = {0, 0, 0, 0, 0};
bool armed[5] = {true, true, true, true, true};
unsigned long lastHitTime[5] = {0, 0, 0, 0, 0};
float currentDrops[5] = {0, 0, 0, 0, 0};
int rawValues[5] = {0, 0, 0, 0, 0};

int currentMode = 0;
unsigned long lastDebugPrint = 0;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  udp.begin(12345);

  Serial.println("\n--- TUNED DRUM STARTUP ---");
  delay(1000); 

  // Initial Calibration
  for(int i=0; i<5; i++) {
    int sum = 0;
    for(int s=0; s<30; s++) { sum += fastTouchRead(pins[i]); delay(2); }
    baselines[i] = (float)sum / 30.0;
  }
}

void loop() {
  unsigned long now = millis();

  for (int i = 0; i < 5; i++) {
    int val = fastTouchRead(pins[i]);
    rawValues[i] = val;

    // 1. DYNAMIC BASELINE TRACKING (The "Magic" part)
    // If the value is HIGHER than baseline, or only slightly lower, it's NOT a touch.
    // We move the baseline to follow the raw value slowly.
    if (val > baselines[i]) {
      baselines[i] = (0.9 * baselines[i]) + (0.1 * (float)val); // Quickly follow upwards
    } else if (baselines[i] - val < (touchSensitivity[i] * 0.8)) {
      baselines[i] = (0.999 * baselines[i]) + (0.001 * (float)val); // Very slowly follow downwards
    }

    currentDrops[i] = baselines[i] - (float)val;

    // 2. HIT DETECTION
    if (armed[i] && currentDrops[i] > touchSensitivity[i]) {
      armed[i] = false;
      lastHitTime[i] = now;
      handleHit(i);
    }

    // 3. RE-ARMING
    if (!armed[i] && currentDrops[i] < (touchSensitivity[i] * 0.8)) {
      if (now - lastHitTime[i] > cooldown) {
        armed[i] = true;
      }
    }
  }

  if (now - lastDebugPrint > 250) {
    printDebugInfo();
    lastDebugPrint = now;
  }
  yield();
}

/*
void loop() {
  unsigned long now = millis();

  for (int i = 0; i < 5; i++) {
    int val = fastTouchRead(pins[i]);
    rawValues[i] = val; 
    currentDrops[i] = baselines[i] - (float)val;

    // --- RE-ARMING / RELEASE CHECK ---
    if (currentDrops[i] < releaseThresholds[i]) {
      // Allow baseline to drift slightly to adapt to temperature/humidity
      baselines[i] = (0.999 * baselines[i]) + (0.001 * (float)val);
      
      if (!armed[i] && (now - lastHitTime[i] > cooldown)) {
        armed[i] = true;
      }
    }

    // --- HIT DETECTION ---
    if (armed[i] && currentDrops[i] > hitThresholds[i]) {
      armed[i] = false;
      lastHitTime[i] = now;
      handleHit(i);
    }
  }

  // Periodic Debug Info
  if (now - lastDebugPrint > 250) {
    printDebugInfo();
    lastDebugPrint = now;
  }
  yield(); 
}
*/

void handleHit(int index) {
  if (index == 4) { 
    currentMode = (currentMode + 1) % 3;
    sendMessage(currentMode == 0 ? "MODE_DRUM" : (currentMode == 1 ? "MODE_CYMBAL" : "MODE_GONG"));
    return;
  }

  if (index == 0) { // Pad A
    if (currentDrops[1] > holdThreshold) sendMessage("DRUM_CENTER_MUTE");
    else sendMessage(currentMode == 0 ? "DRUM_CENTER" : (currentMode == 1 ? "CYMBAL_HIT" : "GONG_HIT"));
  } 
  else if (index == 1) { // Pad B
    if (currentDrops[0] > holdThreshold) sendMessage("DRUM_CENTER_MUTE");
    else sendMessage(currentMode == 0 ? "DRUM_CENTER" : (currentMode == 1 ? "CYMBAL_HIT" : "GONG_HIT"));
  }
  else {
    if (currentMode == 0) sendMessage("DRUM_RIM");
    else if (currentMode == 1) sendMessage("CYMBAL_EDGE");
    else sendMessage("GONG_EDGE");
  }
}

int fastTouchRead(int pin) {
  int sum = 0;
  for (int i = 0; i < 4; i++) sum += touchRead(pin);
  return sum / 4;
}

void sendMessage(const char* msg) {
  if (udp.beginPacket(laptopIP, laptopPort)) {
    udp.print(msg);
    udp.endPacket();
    Serial.print(" >>> "); Serial.println(msg);
  }
}

void printDebugInfo() {
    char buffer[160];
    snprintf(buffer, sizeof(buffer), 
             "DATA M:%d | A:%0.f(%d) B:%0.f(%d) RA:%0.f(%d) RB:%0.f(%d) MOD:%0.f(%d)", 
             currentMode, 
             currentDrops[0], rawValues[0],
             currentDrops[1], rawValues[1],
             currentDrops[2], rawValues[2],
             currentDrops[3], rawValues[3],
             currentDrops[4], rawValues[4]);

    Serial.println(buffer);
    if (udp.beginPacket(laptopIP, laptopPort)) {
      udp.print(buffer);
      udp.endPacket();
    }
}
/*
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

void handleHit(int index);
void sendMessage(const char* msg);
int fastTouchRead(int pin);
void printDebugInfo();

const char* apSSID = "DRUM_PAD_WIFI";
const char* apPassword = "drumpad123";
const char* laptopIP = "192.168.4.2";
const int laptopPort = 5005;
WiFiUDP udp;

// --- INDIVIDUAL SENSITIVITY ---
const int hitDropA = 11;       
const int hitDropB = 7;        
const int hitDropRim = 12;     // Rims
const int holdThreshold = 6;   // Lowered to match Pad B's weak signal
const int releaseDrop = 4;     
const int cooldown = 35;      

const int PIN_CTR_A = 13;
const int PIN_CTR_B = 15; 
const int PIN_RIM_A = 12;
const int PIN_RIM_B = 33;    
const int PIN_MODE  = 27;

float baselines[5] = {0, 0, 0, 0, 0};
bool armed[5] = {true, true, true, true, true};
unsigned long lastHitTime[5] = {0, 0, 0, 0, 0};
int pins[5] = {PIN_CTR_A, PIN_CTR_B, PIN_RIM_A, PIN_RIM_B, PIN_MODE};
float currentDrops[5] = {0, 0, 0, 0, 0};

int currentMode = 0;
unsigned long lastDebugPrint = 0;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  udp.begin(12345);

  Serial.println("\n--- TUNED MUTE DRUM ---");
  delay(1000); 

  for(int i=0; i<5; i++) {
    int sum = 0;
    for(int s=0; s<30; s++) { sum += fastTouchRead(pins[i]); delay(2); }
    baselines[i] = (float)sum / 30.0;
  }
}

void loop() {
  unsigned long now = millis();

  for (int i = 0; i < 5; i++) {
    int val = fastTouchRead(pins[i]);
    currentDrops[i] = baselines[i] - (float)val;

    if (currentDrops[i] < releaseDrop) {
      baselines[i] = (0.9995 * baselines[i]) + (0.0005 * (float)val);
      if (!armed[i] && (now - lastHitTime[i] > cooldown)) armed[i] = true;
    }

    // --- CHECK HITS WITH INDIVIDUAL THRESHOLDS ---
    int currentThreshold = hitDropRim;
    if (i == 0) currentThreshold = hitDropA;
    if (i == 1) currentThreshold = hitDropB;
    if (i == 4) currentThreshold = 15; // Mode pin is usually less sensitive

    if (armed[i] && currentDrops[i] > currentThreshold) {
      armed[i] = false;
      lastHitTime[i] = now;
      handleHit(i);
    }
  }

  if (now - lastDebugPrint > 250) {
    printDebugInfo();
    lastDebugPrint = now;
  }
  yield(); 
}

void handleHit(int index) {
  if (index == 4) { 
    currentMode = (currentMode + 1) % 3;
    sendMessage(currentMode == 0 ? "MODE_DRUM" : (currentMode == 1 ? "MODE_CYMBAL" : "MODE_GONG"));
    return;
  }

  if (index == 0) { // Pad A
    if (currentDrops[1] > holdThreshold) sendMessage("DRUM_CENTER_MUTE");
    else sendMessage(currentMode == 0 ? "DRUM_CENTER" : (currentMode == 1 ? "CYMBAL_HIT" : "GONG_HIT"));
  } 
  else if (index == 1) { // Pad B
    if (currentDrops[0] > holdThreshold) sendMessage("DRUM_CENTER_MUTE");
    else sendMessage(currentMode == 0 ? "DRUM_CENTER" : (currentMode == 1 ? "CYMBAL_HIT" : "GONG_HIT"));
  }
  else {
    if (currentMode == 0) sendMessage("DRUM_RIM");
    else if (currentMode == 1) sendMessage("CYMBAL_EDGE");
    else sendMessage("GONG_EDGE");
  }
}

int fastTouchRead(int pin) {
  int sum = 0;
  for (int i = 0; i < 4; i++) sum += touchRead(pin);
  return sum / 4;
}

void sendMessage(const char* msg) {
  if (udp.beginPacket(laptopIP, laptopPort)) {
    udp.print(msg);
    if (!udp.endPacket()) Serial.println("UDP Send Failed");
    Serial.print(" >>> "); Serial.println(msg);
    // Tiny delay to prevent WiFi buffer flooding
    delay(2); 
  }
}

void printDebugInfo() {
    Serial.print("M:"); Serial.print(currentMode);
    Serial.print(" | A:"); Serial.print(currentDrops[0], 0);
    Serial.print(" B:"); Serial.print(currentDrops[1], 0);
    Serial.print(" | RA:"); Serial.print(currentDrops[2], 0);
    Serial.print(" RB:"); Serial.println(currentDrops[3], 0);
}
/*
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// ---------------- Prototypes ----------------
void handleHit(int index);
void sendMessage(const char* msg);
int fastTouchRead(int pin);
void printDebugInfo(unsigned long now);

// ---------------- Access Point ----------------
const char* apSSID = "DRUM_PAD_WIFI";
const char* apPassword = "drumpad123";
const char* laptopIP = "192.168.4.2";
const int laptopPort = 5005;
WiFiUDP udp;

// ---------------- Settings ----------------
const int hitDrop = 11;       
const int releaseDrop = 5;    
const int cooldown = 30;      

const int PIN_CENTER = 13;
const int PIN_RIM_A  = 12;
const int PIN_RIM_B  = 33;    
const int PIN_MODE   = 27;

float baselines[4] = {0, 0, 0, 0};
bool armed[4] = {true, true, true, true};
unsigned long lastHitTime[4] = {0, 0, 0, 0};
int pins[4] = {PIN_CENTER, PIN_RIM_A, PIN_RIM_B, PIN_MODE};

int currentMode = 0;
unsigned long lastDebugPrint = 0;

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  delay(500);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  udp.begin(12345);

  Serial.println("\n--- DRUM PAD START ---");
  Serial.println("Calibrating... hands off pads!");
  delay(1000); 

  for(int i=0; i<4; i++) {
    int sum = 0;
    for(int s=0; s<30; s++) {
        sum += fastTouchRead(pins[i]);
        delay(5);
    }
    baselines[i] = (float)sum / 30.0;
  }
  Serial.println("Calibration complete. System Ready.");
}

// ---------------- Main Loop ----------------
void loop() {
  unsigned long now = millis();

  for (int i = 0; i < 4; i++) {
    int val = fastTouchRead(pins[i]);
    float drop = baselines[i] - (float)val;

    // 1. DYNAMIC BASELINE
    if (drop < releaseDrop) {
      baselines[i] = (0.9995 * baselines[i]) + (0.0005 * (float)val);
      
      // 2. RE-ARM
      if (!armed[i] && (now - lastHitTime[i] > cooldown)) {
        armed[i] = true;
      }
    }

    // 3. TRIGGER HIT
    if (armed[i] && drop > hitDrop) {
      armed[i] = false;
      lastHitTime[i] = now;
      handleHit(i);
    }
  }

  // 4. PERIODIC DEBUGGING (Every 200ms)
  // This keeps the loop fast while giving you data
  if (now - lastDebugPrint > 200) {
    printDebugInfo(now);
    lastDebugPrint = now;
  }

  yield(); 
}

// ---------------- Helper Functions ----------------

void printDebugInfo(unsigned long now) {
    Serial.print("M:"); Serial.print(currentMode);
    Serial.print(" | C:"); Serial.print(baselines[0] - fastTouchRead(pins[0]), 1);
    Serial.print(" | RA:"); Serial.print(baselines[1] - fastTouchRead(pins[1]), 1);
    Serial.print(" | RB:"); Serial.print(baselines[2] - fastTouchRead(pins[2]), 1);
    Serial.print(" | MOD:"); Serial.println(baselines[3] - fastTouchRead(pins[3]), 1);
}

int fastTouchRead(int pin) {
  int sum = 0;
  for (int i = 0; i < 4; i++) { 
    sum += touchRead(pin);
  }
  return sum / 4;
}

void sendMessage(const char* msg) {
  udp.beginPacket(laptopIP, laptopPort);
  udp.print(msg);
  udp.endPacket();
  
  // Real-time notification of a hit
  Serial.print(" >>> EVENT: ");
  Serial.println(msg);
}

void handleHit(int index) {
  if (index == 3) { // Mode Pin
    currentMode = (currentMode + 1) % 3;
    if (currentMode == 0) sendMessage("MODE_DRUM");
    else if (currentMode == 1) sendMessage("MODE_CYMBAL");
    else sendMessage("MODE_GONG");
    return;
  }

  if (index == 0) { // Center
    if (currentMode == 0) sendMessage("DRUM_CENTER");
    else if (currentMode == 1) sendMessage("CYMBAL_HIT");
    else sendMessage("GONG_HIT");
  } 
  else { // Rim A or Rim B
    if (currentMode == 0) sendMessage("DRUM_RIM");
    else if (currentMode == 1) sendMessage("CYMBAL_EDGE");
    else sendMessage("GONG_EDGE");
  }
}
*/  

/*
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// ---------------- Access Point ----------------
const char* apSSID = "DRUM_PAD_WIFI";
const char* apPassword = "drumpad123";

// Laptop IP when connected to ESP32 AP
const char* laptopIP = "192.168.4.2";
const int laptopPort = 5005;

WiFiUDP udp;
// ---------------------------------------------

// threshold tuning
const int centerHitDrop = 12;
const int centerReleaseDrop = 6;

const int rimHitDrop = 12;
const int rimReleaseDrop = 6;

const int modeHitDrop = 12;
const int modeReleaseDrop = 6;

// baselines
float centerBaseline = 0;
float rimABaseline = 0;
float rimBBaseline = 0;
float modeBaseline = 0;

// armed states
bool centerArmed = true;
bool rimAArmed = true;
bool rimBArmed = true;
bool modeArmed = true;

// 0 = drum, 1 = cymbal, 2 = gong
int mode = 0;

int readTouchSmoothed(int pin) {
  int sum = 0;
  const int samples = 8;
  for (int i = 0; i < samples; i++) {
    sum += touchRead(pin);
    delay(2);
  }
  return sum / samples;
}

float initBaseline(int pin) {
  int sum = 0;
  const int samples = 30;
  for (int i = 0; i < samples; i++) {
    sum += readTouchSmoothed(pin);
    delay(10);
  }
  return sum / (float)samples;
}

void sendMessage(const char* msg) {
  Serial.println(msg);

  udp.beginPacket(laptopIP, laptopPort);
  udp.print(msg);
  udp.endPacket();
}

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);

  Serial.println("Access Point started");
  Serial.print("SSID: ");
  Serial.println(apSSID);
  Serial.print("Password: ");
  Serial.println(apPassword);
  Serial.print("ESP32 AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);
  delay(500);

  startAccessPoint();
  udp.begin(12345);

  delay(1000); // let pads settle before calibration
  centerBaseline = initBaseline(13);
  rimABaseline    = initBaseline(12);
  rimBBaseline = initBaseline(32);
  modeBaseline   = initBaseline(27);

  Serial.println("Calibration complete");
}

void loop() {
  int centerVal = readTouchSmoothed(13);
  int rimAVal    = readTouchSmoothed(12);
  int rimBVal = readTouchSmoothed(2);
  int modeVal   = readTouchSmoothed(27);

  float centerDrop = centerBaseline - centerVal;
  float rimADrop    = rimABaseline - rimAVal;
  float rimBDrop = rimBBaseline - rimBVal;
  float modeDrop   = modeBaseline - modeVal;

  // update baselines only when relaxed
  if (centerDrop < centerReleaseDrop) {
    centerBaseline = 0.995 * centerBaseline + 0.005 * centerVal;
  }

  if (rimADrop < rimReleaseDrop) {
    rimABaseline = 0.995 * rimABaseline + 0.005 * rimAVal;
  }

  if (rimBDrop < rimReleaseDrop) {
    rimBBaseline = 0.995 * rimBBaseline + 0.005 * rimBVal;
  }

  if (modeDrop < modeReleaseDrop) {
    modeBaseline = 0.995 * modeBaseline + 0.005 * modeVal;
  }

  // re-arm
  if (!centerArmed && centerDrop < centerReleaseDrop) {
    centerArmed = true;
  }

  if (!rimAArmed && rimADrop < rimReleaseDrop) {
    rimAArmed = true;
  }

  if (!rimBArmed && rimBDrop < rimReleaseDrop) {
    rimBArmed = true;
  }

  if (!modeArmed && modeDrop < modeReleaseDrop) {
    modeArmed = true;
  }

  // detect new hits
  bool centerNewHit = centerArmed && (centerDrop > centerHitDrop);
  bool rimANewHit    = rimAArmed && (rimADrop > rimHitDrop);
  bool rimBNewHit = rimBArmed && (rimBDrop > rimHitDrop);
  bool modeNewHit   = modeArmed && (modeDrop > modeHitDrop);

  // mode switch
  if (modeNewHit) {
    mode = (mode + 1) % 3;
    modeArmed = false;

    if (mode == 0) {
      sendMessage("MODE_DRUM");
    } else if (mode == 1) {
      sendMessage("MODE_CYMBAL");
    } else {
      sendMessage("MODE_GONG");
    }
  }

  // center / rim events based on mode
  if (centerNewHit) {
    if (mode == 0) {
      sendMessage("DRUM_CENTER");
    } else if (mode == 1) {
      sendMessage("CYMBAL_HIT");
    } else {
      sendMessage("GONG_HIT");
    }
    centerArmed = false;
  }

  if (rimANewHit) {
    if (mode == 0) {
      sendMessage("DRUM_RIM");
    } else if (mode == 1) {
      sendMessage("CYMBAL_EDGE");
    } else {
      sendMessage("GONG_EDGE");
    }
    rimAArmed = false;
  }

  if (rimBNewHit) {
    if (mode == 0) {
      sendMessage("DRUM_RIM");
    } else if (mode == 1) {
      sendMessage("CYMBAL_EDGE");
    } else {
      sendMessage("GONG_EDGE");
    }
    rimBArmed = false;
  }
  
  Serial.print("MODE: ");
  Serial.print(mode);

  Serial.print(" | Center 13 val: ");
  Serial.print(centerVal);
  Serial.print(" base: ");
  Serial.print(centerBaseline);
  Serial.print(" drop: ");
  Serial.print(centerDrop);

  Serial.print(" | Rim A 12 val: ");
  Serial.print(rimAVal);
  Serial.print(" base: ");
  Serial.print(rimABaseline);
  Serial.print(" drop: ");
  Serial.print(rimADrop);

  Serial.print(" | Rim B 33val: ");
  Serial.print(rimBVal);
  Serial.print(" base: ");
  Serial.print(rimBBaseline);
  Serial.print(" drop: ");
  Serial.print(rimBDrop);

  Serial.print(" | Mode 27 val: ");
  Serial.print(modeVal);
  Serial.print(" base: ");
  Serial.print(modeBaseline);
  Serial.print(" drop: ");
  Serial.println(modeDrop);

  delay(5);
}
*/
/*
#include <Arduino.h>

// threshold tuning
const int centerHitDrop    = 12;
const int centerReleaseDrop = 6;

const int rimHitDrop       = 12;
const int rimReleaseDrop   = 6;

const int modeHitDrop      = 12;
const int modeReleaseDrop  = 6;

// baselines
float centerBaseline = 0;
float rimBaseline = 0;
float modeBaseline = 0;

// armed states
bool centerArmed = true;
bool rimArmed = true;
bool modeArmed = true;

// 0 = drum, 1 = cymbal, 2 = gong
int mode = 0;

int readTouchSmoothed(int pin) {
  int sum = 0;
  const int samples = 8;
  for (int i = 0; i < samples; i++) {
    sum += touchRead(pin);
    delay(2);
  }
  return sum / samples;
}

float initBaseline(int pin) {
  int sum = 0;
  const int samples = 30;
  for (int i = 0; i < samples; i++) {
    sum += readTouchSmoothed(pin);
    delay(10);
  }
  return sum / (float)samples;
}

void setup() {
  Serial.begin(115200);
  delay(1000); // let pads settle before calibration

  centerBaseline = initBaseline(13);
  rimBaseline    = initBaseline(12);
  modeBaseline   = initBaseline(27);
}

void loop() {
  int centerVal = readTouchSmoothed(13);
  int rimVal    = readTouchSmoothed(12);
  int modeVal   = readTouchSmoothed(27);

  float centerDrop = centerBaseline - centerVal;
  float rimDrop    = rimBaseline - rimVal;
  float modeDrop   = modeBaseline - modeVal;

  // update baselines only when the pad is relaxed
  if (centerDrop < centerReleaseDrop) {
    centerBaseline = 0.995 * centerBaseline + 0.005 * centerVal;
  }

  if (rimDrop < rimReleaseDrop) {
    rimBaseline = 0.995 * rimBaseline + 0.005 * rimVal;
  }

  if (modeDrop < modeReleaseDrop) {
    modeBaseline = 0.995 * modeBaseline + 0.005 * modeVal;
  }

  // re-arm when relaxed
  if (!centerArmed && centerDrop < centerReleaseDrop) {
    centerArmed = true;
  }

  if (!rimArmed && rimDrop < rimReleaseDrop) {
    rimArmed = true;
  }

  if (!modeArmed && modeDrop < modeReleaseDrop) {
    modeArmed = true;
  }

  // detect new hits
  bool centerNewHit = centerArmed && (centerDrop > centerHitDrop);
  bool rimNewHit    = rimArmed && (rimDrop > rimHitDrop);
  bool modeNewHit   = modeArmed && (modeDrop > modeHitDrop);

  // mode switch
  if (modeNewHit) {
    mode = (mode + 1) % 3;
    modeArmed = false;

    if (mode == 0) {
      Serial.println("MODE_DRUM");
    } else if (mode == 1) {
      Serial.println("MODE_CYMBAL");
    } else {
      Serial.println("MODE_GONG");
    }
  }

  // center / rim events based on mode
  if (centerNewHit) {
    if (mode == 0) {
      Serial.println("DRUM_CENTER");
    } else if (mode == 1) {
      Serial.println("CYMBAL_HIT");
    } else {
      Serial.println("GONG_HIT");
    }
    centerArmed = false;
  }

  if (rimNewHit) {
    if (mode == 0) {
      Serial.println("DRUM_RIM");
    } else if (mode == 1) {
      Serial.println("CYMBAL_EDGE");
    } else {
      Serial.println("GONG_EDGE");
    }
    rimArmed = false;
  }

  // optional debug
  /*
  Serial.print("MODE: ");
  Serial.print(mode);

  Serial.print(" | C val: ");
  Serial.print(centerVal);
  Serial.print(" base: ");
  Serial.print(centerBaseline);
  Serial.print(" drop: ");
  Serial.print(centerDrop);

  Serial.print(" | R val: ");
  Serial.print(rimVal);
  Serial.print(" base: ");
  Serial.print(rimBaseline);
  Serial.print(" drop: ");
  Serial.print(rimDrop);

  Serial.print(" | M val: ");
  Serial.print(modeVal);
  Serial.print(" base: ");
  Serial.print(modeBaseline);
  Serial.print(" drop: ");
  Serial.println(modeDrop);
  */

  //delay(5);
//}
