#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_8x16minimatrix matrix;

#define SDA_PIN 21
#define SCL_PIN 22
#define SWITCH2_PIN 14

#define PM25_TARGET 30.0
#define PM25_HIGH   45.0
#define PM25_MEDIUM 35.0

// Simulated O3 value for prototype testing (ppm)
#define O3_LIMIT 0.12

enum Mode {
  STANDBY,
  AUTO,
  ECO
};

Mode mode = STANDBY;

float pm25 = 30.0;
float ozone = 0.10;
float temperature = 27.0;
float humidity = 60.0;
float power = 32.0;

int fanLevel = 1;
int espLevel = 1;

// Button
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Display
String messages[6];
int messageIndex = 0;
int scrollX = 16;

unsigned long lastScrollTime = 0;
unsigned long messageStartTime = 0;

const unsigned long SCROLL_INTERVAL = 100;
const unsigned long MESSAGE_HOLD = 1000;

bool holdingMessage = false;

void setup() {

  Serial.begin(115200);

  pinMode(SWITCH2_PIN, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);

  matrix.begin(0x70);
  matrix.setRotation(1);
  matrix.setTextSize(1);
  matrix.setTextColor(LED_ON);
  matrix.setTextWrap(false);
  matrix.setBrightness(15);

  matrix.clear();
  matrix.setCursor(0, 0);
  matrix.print("REAIR");
  matrix.writeDisplay();

  delay(1000);

  setStandby();

  Serial.println();
  Serial.println("========== REAIR ==========");
  printValues();
}


void loop() {

  checkButton();

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


// =========================
// STANDBY
// =========================

void setStandby() {

  mode = STANDBY;

  pm25 = 30.0;
  ozone = 0.10;
  temperature = 27.0;
  humidity = 60.0;

  fanLevel = 1;
  espLevel = 1;
  power = 32.0;

  prepareStandbyDisplay();

  Serial.println("MODE: STANDBY");
}


void standbyMode() {

  fanLevel = 1;
  espLevel = 1;
  power = 32.0;
}


// =========================
// BUTTON
// =========================

void checkButton() {

  bool reading = digitalRead(SWITCH2_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime > debounceDelay) {

    if (reading != buttonState) {

      buttonState = reading;

      if (buttonState == LOW) {

        if (mode == STANDBY) {
          startAuto();
        }
      }
    }
  }

  lastButtonState = reading;
}


// =========================
// AUTO
// =========================

void startAuto() {

  mode = AUTO;

  // Simulate dust entering the system
  pm25 = 50.0;

  autoControl();

  prepareAutoDisplay();

  Serial.println();
  Serial.println("========== AUTO ==========");
  Serial.println("Dust event detected");

  printValues();
}


void autoMode() {

  // O3 safety has priority
  if (ozone > O3_LIMIT) {

    fanLevel = 1;
    espLevel = 0;
    power = 10.0;

    prepareSafetyDisplay();

    return;
  }

  autoControl();

  // Wait until the AUTO display finishes scrolling
  if (holdingMessage && messageIndex == 5) {

    pm25 = PM25_TARGET;

    enterEco();
  }
}


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


// =========================
// ECO
// =========================

void enterEco() {

  mode = ECO;

  fanLevel = 1;
  espLevel = 1;

  // Simulated prototype value for ECO mode
  power = 20.0;

  prepareEcoDisplay();

  Serial.println();
  Serial.println("========== ECO ==========");

  printValues();
}


void ecoMode() {

  fanLevel = 1;
  espLevel = 1;

  power = 20.0;
}


// =========================
// DISPLAY PREPARE
// =========================

void prepareStandbyDisplay() {

  messages[0] = "PM2.5 " + String(pm25, 0);
  messages[1] = "POWER " + String(power, 0) + "W";
  messages[2] = "O3 " + String(ozone, 2);
  messages[3] = "TEMP " + String(temperature, 0) + "C";
  messages[4] = "HUM " + String(humidity, 0) + "%";
  messages[5] = "STANDBY";

  resetDisplay();
}


void prepareAutoDisplay() {

  messages[0] = "PM2.5 " + String(pm25, 0);
  messages[1] = "FAN HIGH";
  messages[2] = "ESP HIGH";
  messages[3] = "POWER " + String(power, 0) + "W";
  messages[4] = "O3 " + String(ozone, 2);
  messages[5] = "AUTO";

  resetDisplay();
}


void prepareEcoDisplay() {

  messages[0] = "PM2.5 " + String(pm25, 0);
  messages[1] = "ECO MODE";
  messages[2] = "POWER " + String(power, 0) + "W";
  messages[3] = "FAN LOW";
  messages[4] = "ESP LOW";
  messages[5] = "O3 " + String(ozone, 2);

  resetDisplay();
}


void prepareSafetyDisplay() {

  messages[0] = "O3 HIGH";
  messages[1] = "ESP OFF";
  messages[2] = "PM2.5 " + String(pm25, 0);
  messages[3] = "POWER " + String(power, 0) + "W";
  messages[4] = "SAFETY";
  messages[5] = "AUTO";

  resetDisplay();
}


// =========================
// DISPLAY CONTROL
// =========================

void resetDisplay() {

  messageIndex = 0;

  scrollX = 16;

  holdingMessage = false;

  lastScrollTime = millis();

  drawMessage();
}


void drawMessage() {

  matrix.clear();

  matrix.setCursor(scrollX, 0);

  matrix.print(messages[messageIndex]);

  matrix.writeDisplay();
}


void updateDisplay() {

  // Wait after a message finishes scrolling
  if (holdingMessage) {

    if (millis() - messageStartTime >= MESSAGE_HOLD) {

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


  // Control scrolling speed
  if (millis() - lastScrollTime < SCROLL_INTERVAL) {
    return;
  }

  lastScrollTime = millis();

  drawMessage();

  scrollX--;

  int textWidth = messages[messageIndex].length() * 6;


  // Message has completely left the screen
  if (scrollX < -textWidth) {

    holdingMessage = true;

    messageStartTime = millis();
  }
}


// =========================
// SERIAL MONITOR
// =========================

void printValues() {

  Serial.print("PM2.5      : ");
  Serial.print(pm25);
  Serial.println(" ug/m3");

  Serial.print("O3         : ");
  Serial.print(ozone, 2);
  Serial.println(" ppm");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity   : ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Power      : ");
  Serial.print(power);
  Serial.println(" W");

  Serial.print("Fan Level  : ");
  Serial.println(fanLevel);

  Serial.print("ESP Level  : ");
  Serial.println(espLevel);

  Serial.println("--------------------------");
}