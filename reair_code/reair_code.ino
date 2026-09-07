#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_8x16minimatrix matrix;

// =========================
// PIN
// =========================

#define SDA_PIN 21
#define SCL_PIN 22

#define SWITCH1_PIN 16
#define SWITCH2_PIN 14

// =========================
// AIR QUALITY SETTINGS
// =========================

#define PM25_TARGET 30.0
#define PM25_HIGH   45.0
#define PM25_MEDIUM 35.0

// Simulated O3 value for prototype testing (ppm)
#define O3_LIMIT 0.12

// =========================
// MODE
// =========================

enum Mode {
  STANDBY,
  AUTO,
  ECO
};

Mode mode = STANDBY;

// =========================
// SENSOR VALUES
// =========================

float pm25 = 30.0;
float ozone = 0.10;
float temperature = 27.0;
float humidity = 60.0;
float power = 32.0;

// =========================
// DEVICE LEVEL
// =========================

int fanLevel = 1;
int espLevel = 1;

// =========================
// S1 BUTTON
// =========================

bool lastS1State = HIGH;
bool s1State = HIGH;

unsigned long lastS1Debounce = 0;

const unsigned long DEBOUNCE_TIME = 50;

// =========================
// S2 BUTTON
// =========================

bool lastS2State = HIGH;
bool s2State = HIGH;

unsigned long lastS2Debounce = 0;

// =========================
// MONITORING PAGE
// =========================

int monitorPage = 0;

const int TOTAL_PAGES = 4;

// =========================
// DISPLAY
// =========================

String messages[6];

int messageIndex = 0;
int scrollX = 16;

unsigned long lastScrollTime = 0;
unsigned long messageStartTime = 0;

const unsigned long SCROLL_INTERVAL = 100;
const unsigned long MESSAGE_HOLD = 1000;

bool holdingMessage = false;

// ใช้ตรวจว่า AUTO แสดงจบครบทุกข้อความแล้วหรือยัง
bool autoDisplayFinished = false;

// =========================
// SENSOR UPDATE TIMER
// =========================

unsigned long lastSensorUpdate = 0;

const unsigned long SENSOR_INTERVAL = 1500;

// =========================
// AUTO PM2.5 REDUCTION
// =========================

unsigned long lastDustReduction = 0;

const unsigned long DUST_REDUCTION_INTERVAL = 1200;


// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);

  pinMode(SWITCH1_PIN, INPUT_PULLUP);
  pinMode(SWITCH2_PIN, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);

  matrix.begin(0x70);
  matrix.setRotation(1);
  matrix.setTextSize(1);
  matrix.setTextColor(LED_ON);
  matrix.setTextWrap(false);
  matrix.setBrightness(15);

  // เริ่มต้นจอ
  matrix.clear();
  matrix.setCursor(0, 0);
  matrix.print("REAIR");
  matrix.writeDisplay();

  delay(1000);

  setStandby();

  Serial.println();
  Serial.println("==========================");
  Serial.println("        REAIR SYSTEM");
  Serial.println("==========================");

  printValues();
}


// ======================================================
// LOOP
// ======================================================

void loop() {

  checkS1();
  checkS2();

  if (mode == STANDBY) {

    standbyMode();

  }

  else if (mode == AUTO) {

    autoMode();

  }

  else if (mode == ECO) {

    ecoMode();

  }

  updateDisplay();
}


// ======================================================
// STANDBY
// ======================================================

void setStandby() {

  mode = STANDBY;

  pm25 = 30.0;
  ozone = 0.10;
  temperature = 27.0;
  humidity = 60.0;

  fanLevel = 1;
  espLevel = 1;

  power = 32.0;

  monitorPage = 0;

  prepareStandbyDisplay();

  Serial.println();
  Serial.println("MODE: STANDBY");

}


void standbyMode() {

  // จำลองค่าจากเซนเซอร์ให้มีการเปลี่ยนแปลงเล็กน้อย

  if (millis() - lastSensorUpdate >= SENSOR_INTERVAL) {

    lastSensorUpdate = millis();

    pm25 += random(-2, 3) * 0.1;
    ozone += random(-2, 3) * 0.001;
    temperature += random(-2, 3) * 0.1;
    humidity += random(-2, 3) * 0.2;

    // จำกัดช่วงค่าให้สมจริง
    pm25 = constrain(pm25, 28.0, 33.0);
    ozone = constrain(ozone, 0.08, 0.11);
    temperature = constrain(temperature, 26.0, 28.0);
    humidity = constrain(humidity, 57.0, 63.0);

    fanLevel = 1;
    espLevel = 1;
    power = 32.0;

    Serial.println();
    Serial.println("STANDBY SENSOR UPDATE");
    printValues();
  }
}


// ======================================================
// BUTTON S1
// Monitoring
// ======================================================

void checkS1() {

  bool reading = digitalRead(SWITCH1_PIN);

  if (reading != lastS1State) {

    lastS1Debounce = millis();

  }

  if (millis() - lastS1Debounce > DEBOUNCE_TIME) {

    if (reading != s1State) {

      s1State = reading;

      if (s1State == LOW) {

        nextMonitoringPage();

      }

    }

  }

  lastS1State = reading;
}


// ======================================================
// CHANGE MONITORING PAGE
// ======================================================

void nextMonitoringPage() {

  monitorPage++;

  if (monitorPage >= TOTAL_PAGES) {

    monitorPage = 0;

  }

  prepareMonitoringDisplay();

}


// ======================================================
// BUTTON S2
// Dust Event
// ======================================================

void checkS2() {

  bool reading = digitalRead(SWITCH2_PIN);

  if (reading != lastS2State) {

    lastS2Debounce = millis();

  }

  if (millis() - lastS2Debounce > DEBOUNCE_TIME) {

    if (reading != s2State) {

      s2State = reading;

      if (s2State == LOW) {

        if (mode == STANDBY) {

          startAuto();

        }

      }

    }

  }

  lastS2State = reading;
}


// ======================================================
// AUTO START
// ======================================================

void startAuto() {

  mode = AUTO;

  // จำลองเหตุการณ์ฝุ่นเข้าระบบ
  pm25 = 50.0;

  fanLevel = 3;
  espLevel = 3;

  power = 41.0;

  autoDisplayFinished = false;

  lastDustReduction = millis();

  prepareAutoDisplay();

  Serial.println();
  Serial.println("==========================");
  Serial.println("       DUST EVENT");
  Serial.println("==========================");

  Serial.println("PM2.5 increased!");

  printValues();

}


// ======================================================
// AUTO MODE
// ======================================================

void autoMode() {

  // O3 Safety มีความสำคัญสูงสุด
  if (ozone > O3_LIMIT) {

    fanLevel = 1;
    espLevel = 0;
    power = 10.0;

    prepareSafetyDisplay();

    return;

  }

  // ควบคุม Fan + ESP ตาม PM2.5
  autoControl();

  // จำลองการลดลงของ PM2.5
  reducePM25();

  // เมื่อจอ AUTO แสดงครบทุกข้อความ
  // และ PM2.5 กลับถึงเป้าหมาย
  if (autoDisplayFinished && pm25 <= PM25_TARGET) {

    pm25 = PM25_TARGET;

    enterEco();

  }

}


// ======================================================
// AUTO CONTROL
// ======================================================

void autoControl() {

  if (pm25 > PM25_HIGH) {

    fanLevel = 3;
    espLevel = 3;
    power = 41.0;

  }

  else if (pm25 > PM25_MEDIUM) {

    fanLevel = 2;
    espLevel = 2;
    power = 35.0;

  }

  else {

    fanLevel = 1;
    espLevel = 1;
    power = 32.0;

  }

}


// ======================================================
// SIMULATE PM2.5 REDUCTION
// ======================================================

void reducePM25() {

  if (millis() - lastDustReduction >= DUST_REDUCTION_INTERVAL) {

    lastDustReduction = millis();

    if (pm25 > PM25_TARGET) {

      if (pm25 > PM25_HIGH) {

        pm25 -= random(2, 4);

      }

      else if (pm25 > PM25_MEDIUM) {

        pm25 -= random(1, 3);

      }

      else {

        pm25 -= 0.5;

      }

      if (pm25 < PM25_TARGET) {

        pm25 = PM25_TARGET;

      }

      Serial.print("AUTO PM2.5 = ");
      Serial.println(pm25);

    }

  }

}


// ======================================================
// ECO
// ======================================================

void enterEco() {

  mode = ECO;

  fanLevel = 1;
  espLevel = 1;

  // Low power prototype value
  power = 20.0;

  prepareEcoDisplay();

  Serial.println();
  Serial.println("==========================");
  Serial.println("          ECO MODE");
  Serial.println("==========================");

  printValues();

}


void ecoMode() {

  fanLevel = 1;
  espLevel = 1;

  power = 20.0;

  // จำลองค่าฝุ่นใน ECO
  if (millis() - lastSensorUpdate >= SENSOR_INTERVAL) {

    lastSensorUpdate = millis();

    pm25 += random(-5, 6) * 0.1;
    ozone += random(-2, 3) * 0.001;
    temperature += random(-2, 3) * 0.1;
    humidity += random(-2, 3) * 0.2;

    pm25 = constrain(pm25, 29.0, 32.0);
    ozone = constrain(ozone, 0.08, 0.11);
    temperature = constrain(temperature, 26.0, 28.0);
    humidity = constrain(humidity, 57.0, 63.0);

  }

}


// ======================================================
// DISPLAY - STANDBY
// ======================================================

void prepareStandbyDisplay() {

  messages[0] = "PM2.5 " + String(pm25, 0);
  messages[1] = "POWER " + String(power, 0) + "W";
  messages[2] = "O3 " + String(ozone, 2);
  messages[3] = "TEMP " + String(temperature, 0) + "C";
  messages[4] = "HUM " + String(humidity, 0) + "%";
  messages[5] = "STANDBY";

  resetDisplay();

}


// ======================================================
// DISPLAY - AUTO
// ======================================================

void prepareAutoDisplay() {

  messages[0] = "PM2.5 " + String(pm25, 0);
  messages[1] = "FAN HIGH";
  messages[2] = "ESP HIGH";
  messages[3] = "POWER " + String(power, 0) + "W";
  messages[4] = "O3 " + String(ozone, 2);
  messages[5] = "AUTO";

  autoDisplayFinished = false;

  resetDisplay();

}


// ======================================================
// DISPLAY - ECO
// ======================================================

void prepareEcoDisplay() {

  messages[0] = "PM2.5 " + String(pm25, 0);
  messages[1] = "ECO MODE";
  messages[2] = "POWER " + String(power, 0) + "W";
  messages[3] = "FAN LOW";
  messages[4] = "ESP LOW";
  messages[5] = "O3 " + String(ozone, 2);

  resetDisplay();

}


// ======================================================
// DISPLAY - SAFETY
// ======================================================

void prepareSafetyDisplay() {

  messages[0] = "O3 HIGH";
  messages[1] = "ESP OFF";
  messages[2] = "PM2.5 " + String(pm25, 0);
  messages[3] = "POWER " + String(power, 0) + "W";
  messages[4] = "SAFETY";
  messages[5] = "AUTO";

  resetDisplay();

}


// ======================================================
// DISPLAY - MONITORING
// ======================================================

void prepareMonitoringDisplay() {

  // PAGE 1
  if (monitorPage == 0) {

    messages[0] = "PM2.5 " + String(pm25, 0);
    messages[1] = "TEMP " + String(temperature, 0) + "C";
    messages[2] = "HUM " + String(humidity, 0) + "%";
    messages[3] = "AIR DATA";
    messages[4] = "MONITOR";
    messages[5] = "PAGE 1";

  }

  // PAGE 2
  else if (monitorPage == 1) {

    messages[0] = "FAN LEVEL " + String(fanLevel);
    messages[1] = "ESP LEVEL " + String(espLevel);
    messages[2] = "POWER " + String(power, 0) + "W";
    messages[3] = "CONTROL";
    messages[4] = "MONITOR";
    messages[5] = "PAGE 2";

  }

  // PAGE 3
  else if (monitorPage == 2) {

    messages[0] = "O3 " + String(ozone, 2);
    messages[1] = "O3 LIMIT .12";
    messages[2] = "STATUS SAFE";
    messages[3] = "SAFETY";
    messages[4] = "MONITOR";
    messages[5] = "PAGE 3";

  }

  // PAGE 4
  else {

    messages[0] = "MODE";

    if (mode == STANDBY) {

      messages[1] = "STANDBY";

    }

    else if (mode == AUTO) {

      messages[1] = "AUTO";

    }

    else {

      messages[1] = "ECO";

    }

    messages[2] = "SYSTEM OK";
    messages[3] = "REAIR";
    messages[4] = "MONITOR";
    messages[5] = "PAGE 4";

  }

  resetDisplay();

}


// ======================================================
// RESET DISPLAY
// ======================================================

void resetDisplay() {

  messageIndex = 0;

  scrollX = 16;

  holdingMessage = false;

  lastScrollTime = millis();

  drawMessage();

}


// ======================================================
// DRAW MESSAGE
// ======================================================

void drawMessage() {

  matrix.clear();

  matrix.setCursor(scrollX, 0);

  matrix.print(messages[messageIndex]);

  matrix.writeDisplay();

}


// ======================================================
// UPDATE DISPLAY
// ======================================================

void updateDisplay() {

  // กำลัง Hold ข้อความ
  if (holdingMessage) {

    if (millis() - messageStartTime >= MESSAGE_HOLD) {

      // ถ้าเป็นข้อความสุดท้าย
      if (messageIndex == 5) {

        // AUTO จบครบทุกข้อความ
        if (mode == AUTO) {

          autoDisplayFinished = true;

        }

      }

      messageIndex++;

      if (messageIndex >= 6) {

        messageIndex = 0;

      }

      scrollX = 16;

      holdingMessage = false;

      drawMessage();

    }

    return;

  }


  // ความเร็ว Scroll
  if (millis() - lastScrollTime < SCROLL_INTERVAL) {

    return;

  }

  lastScrollTime = millis();

  drawMessage();

  scrollX--;

  int textWidth = messages[messageIndex].length() * 6;


  // ข้อความเลื่อนออกจากจอหมดแล้ว
  if (scrollX < -textWidth) {

    holdingMessage = true;

    messageStartTime = millis();

  }

}
// ======================================================
// SERIAL MONITOR
// ======================================================

void printValues() {

  Serial.print("PM2.5      : ");
  Serial.print(pm25, 1);
  Serial.println(" ug/m3");

  Serial.print("O3         : ");
  Serial.print(ozone, 3);
  Serial.println(" ppm");

  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" C");

  Serial.print("Humidity   : ");
  Serial.print(humidity, 1);
  Serial.println(" %");

  Serial.print("Power      : ");
  Serial.print(power, 1);
  Serial.println(" W");

  Serial.print("Fan Level  : ");
  Serial.println(fanLevel);

  Serial.print("ESP Level  : ");
  Serial.println(espLevel);

  Serial.println("--------------------------");

}