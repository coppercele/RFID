
#include <Arduino.h>
#include "MFRC522_I2C.h"
#include <M5Stack.h>
#include "WiFi.h"
#include "esp_wps.h"
#include <ArduinoJson.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#define LGFX_AUTODETECT
#include <LGFX_AUTODETECT.hpp>

MFRC522 mfrc522(0x28); // Create MFRC522 instance.  创建MFRC522实例

static LGFX lcd;
static LGFX_Sprite sprite(&lcd);
// DynamicJsonDocument doc(1024);

char s[20];
struct tm timeInfo;

struct beans {
  int day = 0;
  int hour = 0;
  int minute = 0;
  int dateIndex = 2;
  int mode = 0;
  char *uidChar;
  byte uidSize = 0;
  bool isSdEnable = false;
  bool isWifiEnable = false;
  File fileDataJson;
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
    // for (int i = 0; i < data.uidSize; i++) {
    sprite.printf("%s", data.uidChar);
    // }
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

  sprite.setTextSize(1);
  sprite.setCursor(280, 190);
  sprite.printf("SD");
  sprite.setCursor(280, 210);
  sprite.printf("%s", data.isSdEnable ? "OK" : "NG");

  sprite.pushSprite(&lcd, 0, 0);
}

int searchNewestId(JsonDocument &jsonDocument) {

  JsonArray array = jsonDocument["json"].as<JsonArray>();
  int size = array.size();
  // Serial.printf("array size:%d\n", size);

  // 配列の最後の要素を取得
  JsonObject object = array[size - 1];

  const char *json_item_id = object["id"];

  int jsonIdInt = 9;
  sscanf(json_item_id, "%d", &jsonIdInt);

  // Serial.printf("jsonId:%s\n", json_item_id);
  // Serial.printf("jsonIdInt:%d\n", jsonIdInt);
  return jsonIdInt;
}

void setup() {
  M5.begin(true, true, true, true); // Init M5Stack.  初始化M5Stack
  M5.Power.begin();                 // Init power  初始化电源模块
  // Wire.begin(); // Wire init, adding the I2C bus.  Wire初始化, 加入i2c总线
  delay(500);

  File f = SD.open("/property.json");
  if (f) {
    data.isSdEnable = true;
    Serial.printf("SD Card Found\n");
  }
  else {
    Serial.printf("SD Card Not Found\n");
  }
  f.close();

  data.fileDataJson = SD.open("/data.json");

  mfrc522.PCD_Init(); // Init MFRC522.  初始化 MFRC522
                      // M5.Lcd.println("Please put the card\n\nUID:");
  lcd.init();
  lcd.setRotation(1);

  lcd.setBrightness(200);

  sprite.setPsram(true);
  sprite.setColorDepth(8);
  sprite.createSprite(320, 240);
  makeSprite();

  Serial.print("setup json check.\n");

  File f2 = SD.open("/data.json");

  DynamicJsonDocument jsonDocument(1024);
  DeserializationError error = deserializeJson(jsonDocument, f2);

  if (error) {
    // data.jsonが空

    Serial.print("data.json is empty.\n");
    Serial.println(error.c_str());
  }
  else {
    Serial.print("deserialized:\n");
    serializeJsonPretty(jsonDocument, Serial);
    Serial.println();
    int i = searchNewestId(&jsonDocument);
    Serial.printf("newestId:%d\n", i);
  }
  f2.close();

  // WiFi.begin();

  // int count = 0;
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500); // 500ms毎に.を表示
  //   Serial.print(".");
  //   count++;
  //   if (count == 10) {
  //     //   // 5秒間待ってからWPSを開始する
  //     //   // 以下サンプルそのまま
  //     //   WiFi.onEvent(WiFiEvent);
  //     //   WiFi.mode(WIFI_MODE_STA);

  //     //   Serial.println("Starting WPS");
  //     //   wpsInitConfig();
  //     //   esp_wifi_wps_enable(&config);
  //     // Serial.println(esp_wifi_wps_start(0));
  //     Serial.println("Wifi failed.\n");
  //     break;
  //   }
  // }
  // if (WiFi.status()) {
  //   Serial.println("\nConnected");
  //   configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com",
  //              "ntp.jst.mfeed.ad.jp");
  //   data.isWifiEnable = true;
  // }
}

void writeJson(int id, char *uid, DynamicJsonDocument jsonDocument) {
  //       char timeStr[20];
  //       getLocalTime(&timeInfo);

  //         Serial.print("deserialized:");
  //         serializeJsonPretty(jsonDocument, Serial);
  //         Serial.println();

  // JsonArray json = jsonDocument.createNestedArray();
  // JsonObject json_0 = json.createNestedObject();
  // json_0["id"] = "000";

  // json_0["uid"] = buf;
  // sprintf(timeStr, " %04d/%02d/%02d %02d:%02d:%02d",
  //         timeInfo.tm_year + 1900, timeInfo.tm_mon + 1,
  //         timeInfo.tm_mday, timeInfo.tm_hour, timeInfo.tm_min,
  //         timeInfo.tm_sec);
  // Serial.printf("time:%s\n", timeStr);
  // json_0["scandate"] = timeStr;

  // time_t expireDate = mktime(&timeInfo);

  // expireDate += data.minute * 60;
  // expireDate += data.hour * 3600;
  // expireDate += data.day * 24 * 60 * 60;

  // sprintf(timeStr, "%d", expireDate);
  // json_0["expire"] = timeStr;

  JsonArray json = jsonDocument.createNestedArray("json");

  JsonObject json_0 = json.createNestedObject();
  char buf[4];
  sprintf(buf, "%03s", id);
  json_0["id"] = buf;
  json_0["uid"] = data.uidChar;
  json_0["scandate"] = " 2022/11/23 03:33:33";
  json_0["expire"] = "1669401213";

  serializeJsonPretty(jsonDocument, Serial);
  Serial.println();
  // serializeJsonPretty(jsonDocument, data.fileDataJson);
  // Serial.printf("JSON Wrote to SD Card\n");
}

void loop() {
  M5.update();

  if (!mfrc522.PICC_IsNewCardPresent() ||
      !mfrc522.PICC_ReadCardSerial()) { // 如果没有读取到新的卡片
    delay(200);
    return;
  }
  else {
    data.uidSize = mfrc522.uid.size;
    char buf[9];

    sprintf(buf, "%02X%02X%02X%02X", mfrc522.uid.uidByte[0],
            mfrc522.uid.uidByte[1], mfrc522.uid.uidByte[2],
            mfrc522.uid.uidByte[3]);
    data.uidChar = buf;

    if (data.isSdEnable) {

      if (data.fileDataJson) {

        DynamicJsonDocument jsonDocument(1024);
        DeserializationError error =
            deserializeJson(jsonDocument, data.fileDataJson);

        if (error) {
          // data.jsonが空

          Serial.print("data.json is empty.\n");
          Serial.println(error.c_str());

          writeJson(0, data.uidChar, jsonDocument);
        }
        else {
          // data.jsonにデータがある
          Serial.println("JSON data found\n");
          for (JsonObject property_item :
               jsonDocument["json"].as<JsonArray>()) {

            const char *uid = property_item["uid"];

            Serial.printf("new uid:%s\n", data.uidChar);
            Serial.printf("json uid:%s\n", uid);

            if (!strcmp(uid, data.uidChar)) {
              // スキャンされたuidが既に存在する
              // 画面を更新して終了
              Serial.printf("UID :%s already exist.\n", data.uidChar);
              makeSprite();
              return;
            }
          }

          // データが見つからなかったのでIDを増やして追加
          JsonArray array = jsonDocument["json"].as<JsonArray>();
          int size = array.size();

          JsonObject object = array[size - 1];

          const char *json_item_id = object["id"];
          // JsonObject object = array.createNestedObject();

          int jsonIdInt = 9;
          sscanf(json_item_id, "%d", jsonIdInt);

          Serial.printf("jsonId:%d\n");

          char buf[4];
          sprintf(buf, "%03s", jsonIdInt);
          object["id"] = buf;
          object["uid"] = data.uidChar;
          object["scandate"] = " 2022/11/23 03:33:33";
          object["expire"] = "1669401213";

          serializeJsonPretty(jsonDocument, Serial);
          Serial.println();
        }
      }
      else {
        Serial.printf("SD Card Not Found\n");
      }
      makeSprite();
    }
  }

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
  delay(1);
}