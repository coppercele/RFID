
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

MFRC522 mfrc522(0x28);

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
  char message[64];
  byte uidSize = 0;
  bool isSdEnable = false;
  bool isWifiEnable = false;
  time_t time;
} data;

void makeSprite() {

  sprite.clear(TFT_BLACK);
  sprite.setFont(&fonts::lgfxJapanGothicP_20);
  sprite.setTextSize(1);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  if (data.message != NULL) {
    sprite.setCursor(0, 0);
    // for (int i = 0; i < data.uidSize; i++) {
    sprite.printf("%s", data.message);
    // }
  }

  sprite.setCursor(0, 150);
  sprite.setTextSize(3);
  sprite.printf(" ");
  for (int i = 0; i < data.dateIndex; i++) {
    sprite.printf("　  ");
  }

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
  case 2:
    sprite.printf("%s", "  確認　↑ 　←");
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

// 既存のJSONを検索して一番大きいidを返す
int searchNewestId(JsonDocument &jsonDocument) {

  // "json"をrootにする配列を取得
  JsonArray array = jsonDocument["json"].as<JsonArray>();
  int size = array.size();
  // Serial.printf("array size:%d\n", size);

  // 配列の最後の要素を取得
  JsonObject object = array[size - 1];
  // id(文字列)を取得してintに変換
  const char *json_item_id = object["id"];
  int jsonIdInt = 9;
  sscanf(json_item_id, "%d", &jsonIdInt);

  // Serial.printf("jsonId:%s\n", json_item_id);
  // Serial.printf("jsonIdInt:%d\n", jsonIdInt);
  return jsonIdInt;
}

// 新しいJsonDocumentレコードを作成する
void createNewRecord(JsonDocument &doc, int id, char *uid) {
  // 追加する要素を作成

  char timeStr[20];
  getLocalTime(&timeInfo);

  char buf[4];
  sprintf(buf, "%03d", id);
  doc["id"] = buf;
  doc["uid"] = uid;

  sprintf(timeStr, "%04d/%02d/%02d %02d:%02d:%02d", timeInfo.tm_year + 1900,
          timeInfo.tm_mon + 1, timeInfo.tm_mday, timeInfo.tm_hour,
          timeInfo.tm_min, timeInfo.tm_sec);
  Serial.printf("time:%s\n", timeStr);
  doc["scandate"] = timeStr;

  time_t expireDate = mktime(&timeInfo);

  expireDate += data.minute * 60;
  expireDate += data.hour * 3600;
  expireDate += data.day * 24 * 60 * 60;

  sprintf(timeStr, "%d", expireDate);
  doc["expire"] = timeStr;
}

void setup() {
  M5.begin(true, true, true, true);
  M5.Power.begin();
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

  mfrc522.PCD_Init();

  lcd.init();
  lcd.setRotation(1);

  lcd.setBrightness(128);

  sprite.setPsram(true);
  sprite.setColorDepth(8);
  sprite.createSprite(320, 240);
  makeSprite();

  Serial.print("setup json check.\n");

  // SDカードから読み込む
  f = SD.open("/data.json");

  DynamicJsonDocument jsonDocument(1024);
  // deseriarizeする
  DeserializationError error = deserializeJson(jsonDocument, f);
  f.close();
  f = SD.open("/data.json");

  if (error) {
    // data.jsonが空

    Serial.print("data.json is empty.\n");
    Serial.println(error.c_str());
    f.close();
  }
  else {
    // deseriarize成功
    Serial.print("deserialized:\n");
    serializeJsonPretty(jsonDocument, Serial);
    Serial.println();
    f.close();
  }

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

  getLocalTime(&timeInfo);
  data.time = mktime(&timeInfo);
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    switch (data.mode) {
    case 0:
      data.mode = 1;
      break;
    case 1:
      data.mode = 2;
      break;
    case 2:
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

  getLocalTime(&timeInfo);
  time_t now = mktime(&timeInfo);

  if (10 <= now - data.time) {
    Serial.printf("10sec %ld\n", now);
    data.time = now;
    File f = SD.open("/data.json");
    DynamicJsonDocument jsonDocument(1024);
    DeserializationError error = deserializeJson(jsonDocument, f);
    if (!error) {
      JsonArray array = jsonDocument["json"].as<JsonArray>();
      for (int i = 0; i < array.size(); i++) {
        JsonObject object = array[i];

        const char *unixTime = object["expire"];

        long lUnixTime;
        sscanf(unixTime, "%ld", lUnixTime);

        if (lUnixTime < now) {
          // 期限が切れ表示
          const char *id = object["id"];
          const char *uid = object["uid"];
          const char *scandate = object["scandate"];
          const char *expire = object["expire"];
          const char *unixTime = object["expire"];

          long lUnixTime;
          sscanf(unixTime, "%ld", lUnixTime);
          struct tm *timeInfo = localtime((time_t *)&lUnixTime);
          char timeStr[20];
          sprintf(timeStr, "%04d/%02d/%02d %02d:%02d:%02d",
                  timeInfo->tm_year + 1900, timeInfo->tm_mon + 1,
                  timeInfo->tm_mday, timeInfo->tm_hour, timeInfo->tm_min,
                  timeInfo->tm_sec);
          Serial.printf("expire time:%s\n", timeStr);

          Serial.printf("id:%s uid:%s scandate:%s expire:%s\n", id, uid,
                        scandate, expire);
          sprintf(data.message,
                  "期限が切れています\nid:%s\nuid:%s\nscandate:%s\nexpire:%s\n",
                  id, uid, scandate, timeStr);
        }
      }
    }

    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      delay(200);
      return;
    }
    data.uidSize = mfrc522.uid.size;
    char buf[9];

    sprintf(buf, "%02X%02X%02X%02X", mfrc522.uid.uidByte[0],
            mfrc522.uid.uidByte[1], mfrc522.uid.uidByte[2],
            mfrc522.uid.uidByte[3]);
    data.uidChar = buf;

    if (!data.isSdEnable) {
      return;
    }

    File f = SD.open("/data.json");
    if (!f) {
      Serial.printf("/data.json Not Found\n");
      return;
    }

    DynamicJsonDocument jsonDocument(1024);
    DeserializationError error = deserializeJson(jsonDocument, f);
    // deserializeするとFileに書き込めなくなるので開きなおす
    f.close();
    f = SD.open("/data.json");
    if (error) {
      // data.jsonが空

      Serial.print("data.json is empty.\n");
      Serial.println(error.c_str());
      // json配列を作成
      JsonArray json = jsonDocument.createNestedArray("json");
      DynamicJsonDocument doc(200);
      createNewRecord(doc, 0, data.uidChar);
      // json配列に要素を追加
      json.add(doc);
      // SDカードに書きこむ
      serializeJsonPretty(jsonDocument, f);
      Serial.print("JSON Wrote to SD Card\n");
      makeSprite();
      return;
      // writeJson(0, data.uidChar, jsonDocument, f);
    }
    // data.jsonにデータがある
    Serial.println("JSON data found\n");

    JsonArray array = jsonDocument["json"].as<JsonArray>();
    for (int i = 0; i < array.size(); i++) {
      JsonObject object = array[i];
      const char *uid = object["uid"];
      Serial.printf("new uid:%s\n", data.uidChar);
      Serial.printf("json uid:%s\n", uid);
      if (!strcmp(uid, data.uidChar)) {
        // スキャンされたuidが既に存在する
        switch (data.mode) {
        case 0:
          // 画面を更新して終了
          sprintf(data.message, "UID:%s\n既に存在します", data.uidChar);
          Serial.printf("UID :%s already exist.\n", data.uidChar);
          break;
        case 1:
          // 削除
          array.remove(i);
          Serial.println("Matched record removed.");
          // jsonを表示
          serializeJsonPretty(jsonDocument, Serial);
          Serial.println();

          sprintf(data.message, "UID:%s\n削除されました", data.uidChar);
          // // SDカードに書きこむ
          // serializeJsonPretty(jsonDocument, f);
          // Serial.print("JSON Wrote to SD Card\n");
          break;
        case 2:
          // 確認
          {
            const char *id = object["id"];
            const char *uid = object["uid"];
            const char *scandate = object["scandate"];
            const char *expire = object["expire"];
            const char *unixTime = object["expire"];

            long lUnixTime;
            sscanf(unixTime, "%ld", lUnixTime);
            struct tm *timeInfo = localtime((time_t *)&lUnixTime);
            char timeStr[20];
            sprintf(timeStr, "%04d/%02d/%02d %02d:%02d:%02d",
                    timeInfo->tm_year + 1900, timeInfo->tm_mon + 1,
                    timeInfo->tm_mday, timeInfo->tm_hour, timeInfo->tm_min,
                    timeInfo->tm_sec);
            Serial.printf("expire time:%s\n", timeStr);

            Serial.printf("id:%s uid:%s scandate:%s expire:%s\n", id, uid,
                          scandate, expire);
            sprintf(data.message,
                    "登録されています\nid:%s\nuid:%s\nscandate:%s\nexpire:%s\n",
                    id, uid, scandate, timeStr);
          }

          break;

        default:
          break;
        }
        makeSprite();
        delay(1000);
        return;
      }

      // データが見つからなかった場合
      switch (data.mode) {
      case 0:
        // 追加
        // データが見つからなかったのでIDを増やして追加
        {
          // 一番古いidを取得
          int newestId = searchNewestId(jsonDocument);
          Serial.printf("newestId:%d\n", newestId);

          // 追加する要素を作成
          DynamicJsonDocument doc(200);
          createNewRecord(doc, newestId + 1, data.uidChar);

          // rootを"json"とする配列を取得
          JsonArray jsonArray = jsonDocument["json"].as<JsonArray>();
          // データを追加
          jsonArray.add(doc);
          // jsonを表示
          serializeJsonPretty(jsonDocument, Serial);
          Serial.println();
          // SDカードに書きこむ
          serializeJsonPretty(jsonDocument, f);
          Serial.print("JSON Wrote to SD Card\n");
        }
        break;
      case 1:
        // 削除：見つかりませんでした
        Serial.println("UID not found.");
        sprintf(data.message, "UID:%s\n登録されていません", data.uidChar);

        break;
      case 2:
        // 確認：見つかりませんでした
        Serial.println("UID not found.");
        sprintf(data.message, "UID:%s\n登録されていません", data.uidChar);
        break;
      default:
        break;
      }
    }
    makeSprite();

    delay(1);
  }