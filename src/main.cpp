
// define ENABLE_WIFI 1  // remove to disable wifi functions
// You will need this: https://github.com/pauls-3d-things/node-mini-iot-server

#ifdef ENABLE_WIFI
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#endif
#include <U8g2lib.h>
#include <Wire.h>

#include "Arduino.h"
#include "config.h"

#define DISP_REFRESH_DELAY 1000 / 10
#define CLICK_DELAY 250
#define TIME_MIN_SECONDS 1
#define TIME_MAX_SECONDS 5

U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, D5, D4, D2);

const int button = D8;  // the number of the pushbutton pin

enum GameState { START, RUNNING, FINISHED };
uint8_t gameState = START;

#ifdef ENABLE_WIFI
void waitForWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.hostname(HOSTNAME);
    WiFi.mode(WIFI_STA);
    do {
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      delay(4000);
    } while (WiFi.status() != WL_CONNECTED);
  }
}

void postData(String filename, String payload, boolean append, boolean tsprefix) {
  HTTPClient http;
  http.begin(String("http://") + MINI_IOT_SERVER         //
             + "/files/" + HOSTNAME                      //
             + "/" + filename                            //
             + "?append=" + (append ? "true" : "false")  //
             + "&tsprefix=" + (tsprefix ? "true" : "false"));
  http.addHeader("Content-Type", "text/plain");
  http.POST(payload);
  // http.writeToStream(&Serial);
  http.end();
}
#endif

void setup() {
  Serial.begin(115200);

  pinMode(button, INPUT);

  u8g2.begin();
  u8g2.setContrast(0);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_profont12_tf);
  u8g2.drawStr(1, 10, "Connecting...");
  u8g2.sendBuffer();

#ifdef ENABLE_WIFI
  waitForWifi();
#endif
}

void loop() {
  static unsigned long lastDisplayRefresh = 0;
  static unsigned long lastClick = 0;
  static unsigned long gameStart = 0;
  static boolean highScoreSent = false;
  static long nextDuration = 0;
  static long timeDiff = 0;

  unsigned long delta;
  unsigned long durationMillis;

  if (nextDuration == 0) {
    nextDuration = random(TIME_MIN_SECONDS, TIME_MAX_SECONDS + 1);
  }

  int buttonState = digitalRead(button);  // button pressed = HIGH

  if (lastClick + CLICK_DELAY < millis() && buttonState == HIGH) {
    switch (gameState) {
      case START:
        gameStart = millis();
        gameState = RUNNING;
        highScoreSent = false;
        break;
      case RUNNING:
        delta = millis() - gameStart;
        durationMillis = nextDuration * 1000;
        if (delta > durationMillis) {
          timeDiff = delta - durationMillis;
        } else {
          timeDiff = -1 * (durationMillis - delta);
        }
        Serial.println(timeDiff);
        gameState = FINISHED;
        break;
      case FINISHED:
        gameState = START;
        nextDuration = random(TIME_MIN_SECONDS, TIME_MAX_SECONDS + 1);
        break;
      default:
        break;
    }
    lastClick = millis();
  }

  if (lastDisplayRefresh + DISP_REFRESH_DELAY < millis() || gameState == FINISHED) {
    u8g2.clearBuffer();
    switch (gameState) {
      case START:
        u8g2.setFont(u8g2_font_profont12_tf);
        u8g2.drawStr(1, 10, "TimeMaster v1.0");
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.drawStr(16, 27, (String("Next: ") + String(nextDuration) + "s").c_str());
        break;
      case RUNNING:
        u8g2.setFont(u8g2_font_profont12_tf);
        u8g2.drawStr(1, 10, "Running ...");
        break;
      case FINISHED:
        u8g2.setFont(u8g2_font_profont12_tf);
        if (timeDiff == 0) {
          u8g2.drawStr(1, 10, "You did it!!!");
        } else if (abs(timeDiff) < 100) {
          u8g2.drawStr(1, 10, "Almost!!");
        } else if (abs(timeDiff) < 500) {
          u8g2.drawStr(1, 10, "Close!!!");
        } else if (abs(timeDiff) < 1000) {
          u8g2.drawStr(1, 10, "Not bad...");
        } else if (abs(timeDiff) > 10000) {
          u8g2.drawStr(1, 10, "Time to practice......");
        } else {
          u8g2.drawStr(1, 10, "Better luck next time!...");
        }

        u8g2.setFont(u8g2_font_profont22_tf);
        if (timeDiff == 0) {
          u8g2.drawStr(16, 27, "TimeMaster!");
        } else if (timeDiff < 0) {
          u8g2.drawStr(16, 27, (String(timeDiff) +  "ms").c_str());
        } else {
          u8g2.drawStr(16, 27, (String("+") + String(timeDiff) + "ms").c_str());
        }
        break;
      default:
        break;
    }

    u8g2.sendBuffer();
    lastDisplayRefresh = millis();
  }

  if (gameState == FINISHED && !highScoreSent) {
#ifdef ENABLE_WIFI
    waitForWifi();
    postData("highScore.csv", String(timeDiff), true, true);
    highScoreSent = true;
#endif
  }

  if (gameState == FINISHED && millis() - lastClick < 2000) {
    // allow us to hold the button at the end for a bit
    delay(2000);
  }
  delay(1);
}
