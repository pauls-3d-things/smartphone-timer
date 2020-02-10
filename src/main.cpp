#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "Arduino.h"
#include "config.h"

U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, D5, D4, D2);
unsigned long timestamp_start = 0;
unsigned long timestamp_diff = 0;
int secs = 0;
int mins = 0;
int hours = 0;
boolean start = true;

const int button = D8;  // the number of the pushbutton pin

// variables will change:
int buttonState = 0;  // variable for reading the pushbutton status

void waitForWifi() {
  WiFi.hostname(HOSTNAME);
  WiFi.mode(WIFI_STA);
  do {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(4000);
  } while (WiFi.status() != WL_CONNECTED);
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

String twoDigits(int x) { return (x < 10 ? String("0") : String("")) + String(x); }

const String format(int hours, int mins, int secs) {
  return twoDigits(hours) + String(":") + twoDigits(mins) + String(":") + twoDigits(secs);
}

void setup() {
  Serial.begin(115200);

  pinMode(button, INPUT);

  u8g2.begin();
  u8g2.setContrast(0);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_profont12_tf);
  u8g2.drawStr(1, 10, "Connecting...");
  u8g2.sendBuffer();

  waitForWifi();
}

void loop() {
  buttonState = digitalRead(button);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    if (start) {
      timestamp_start = millis();
      postData("phone.csv", "0", true, true);
      postData("phone.csv", "1", true, true);
      start = false;
    }

    timestamp_diff = millis() - timestamp_start;
    secs = (int)(timestamp_diff / 1000) % 60;
    mins = (int)((timestamp_diff / (1000 * 60)) % 60);
    hours = (int)((timestamp_diff / (1000 * 60 * 60)) % 24);
    Serial.println(format(hours, mins, secs));
  } else {
    if (!start) {
      postData("phone.csv", "1", true, true);
      postData("phone.csv", "0", true, true);
    }
    start = true;
    Serial.println("Low");
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_profont12_tf);
  u8g2.drawStr(1, 10, "Smartphone Timer v1.0");
  u8g2.setFont(u8g2_font_profont22_tf);
  u8g2.drawStr(16, 27, (format(hours, mins, secs)).c_str());
  u8g2.sendBuffer();
  delay(1000);
}
