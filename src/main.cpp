
#include <Arduino.h>
#include "MFRC522_I2C.h"
#include <M5Stack.h>
#include <ArduinoJson.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#define LGFX_AUTODETECT
#include <LGFX_AUTODETECT.hpp>

MFRC522 mfrc522(0x28); // Create MFRC522 instance.  创建MFRC522实例

static LGFX lcd;
static LGFX_Sprite sprite(&lcd);
DynamicJsonDocument doc(1024);

struct beans {
  int day = 0;
  int hour = 0;
  int minute = 0;
  int dateIndex = 2;
  int mode = 0;
  char uidChar[8];
  byte uidSize = 0;
} data;

void makeSprite() {

  sprite.clear(TFT_BLACK);
  sprite.setFont(&fonts::lgfxJapanGothicP_20);
  sprite.setTextSize(1);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite.setTextSize(2);
  if (data.uidSize != 0) {
    sprite.setCursor(0, 0);
    sprite.printf("ID: ");
    for (int i = 0; i < data.uidSize; i++) {
      sprite.printf("%02X ", data.uidChar[i]);
    }
  }

  sprite.setCursor(0, 150);
  sprite.printf("  ");
  for (int i = 0; i < data.dateIndex; i++) {
    sprite.printf("　　 ");
  }
  sprite.setTextSize(3);
  sprite.printf("%s", "■");

  sprite.setTextSize(2);
  sprite.setCursor(0, 190);
  switch (data.mode) {
  case 0:
    sprite.printf("%s", "  登録　↑ 　←");
    break;
  case 1:
    sprite.printf("%s", "  削除　↑ 　←");
    break;

  default:
    break;
  }

  sprite.setCursor(0, 130);
  sprite.printf("  %02d日%02d時%02d分", data.day, data.hour, data.minute);

  sprite.pushSprite(&lcd, 0, 0);
}

void setup() {
  M5.begin();       // Init M5Stack.  初始化M5Stack
  M5.Power.begin(); // Init power  初始化电源模块
  Wire.begin(); // Wire init, adding the I2C bus.  Wire初始化, 加入i2c总线

  mfrc522.PCD_Init(); // Init MFRC522.  初始化 MFRC522
                      // M5.Lcd.println("Please put the card\n\nUID:");
  lcd.init();
  lcd.setRotation(1);

  lcd.setBrightness(200);

  sprite.setPsram(true);
  sprite.setColorDepth(8);
  sprite.createSprite(320, 240);
  makeSprite();

  byte test[4] = {0x76, 0x4d, 0x69, 0x25};
  char uid[mfrc522.uid.size];
  sscanf(data.uidChar, "%0X", uid);
  // String str = String(data.uidByte[0],HEX);
  // str.concat
  Serial.printf("uid=%02x%02x%02x%02x\n", uid[0], uid[1], uid[2], uid[3]);
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    switch (data.mode) {
    case 0:
      data.mode = 1;
      break;
    case 1:
      data.mode = 0;
      break;

    default:
      break;
    }
    makeSprite();
  }

  if (M5.BtnC.wasPressed()) {
    if (data.dateIndex == 0) {
      data.dateIndex = 2;
    }
    else {
      data.dateIndex--;
    }

    makeSprite();
  }
  if (M5.BtnB.wasPressed()) {
    switch (data.dateIndex) {
    case 2:
      if (59 <= data.minute) {
        data.minute = 0;
      }
      else {
        data.minute++;
      }
      break;
    case 1:
      if (12 <= data.hour) {
        data.hour = 0;
      }
      else {
        data.hour++;
      }
      break;
    case 0:
      if (30 < data.day) {
        data.day = 0;
      }
      else {
        data.day++;
      }
      break;

    default:
      break;
    }
    makeSprite();
  }

  if (!mfrc522.PICC_IsNewCardPresent() ||
      !mfrc522.PICC_ReadCardSerial()) { // 如果没有读取到新的卡片
    delay(200);
    return;
  }
  else {
    data.uidSize = mfrc522.uid.size;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      data.uidChar[i] = mfrc522.uid.uidByte[i];
    }

    JsonArray json = doc.createNestedArray("json");
    JsonObject json_0 = json.createNestedObject();
    json_0["id"] = "000";
    // json_0["uid"] = uid;
    json_0["expire"] = "2020/01/01 24:59";
    serializeJson(doc, Serial);
    Serial.println();
    makeSprite();
  }

  delay(1);
}