
#include <Arduino.h>
#include "MFRC522_I2C.h"
#include <M5Stack.h>
#include "WiFi.h"
// #include "esp_wps.h"
#include "wpsConnector.h"
#include <ArduinoJson.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#define LGFX_AUTODETECT
#include <LGFX_AUTODETECT.hpp>

MFRC522 mfrc522(0x28);

static LGFX lcd;
static LGFX_Sprite sprite(&lcd);

// char s[20];
struct tm timeInfo;

struct beans {
  int day = 0;
  int hour = 0;
  int minute = 10;
  int dateIndex = 2;
  int mode = 0;
  char *uidChar;
  char message[128];
  byte uidSize = 0;
  bool isSdEnable = false;
  bool isWifiEnable = false;
  unsigned long time;
  bool isExpired = false;
} data;

// Spriteを構築して画面を更新する
void makeSprite() {
  sprite.setFont(&fonts::lgfxJapanGothicP_20);
  sprite.setTextSize(1);

  if (data.isExpired) {
    sprite.clear(TFT_RED);
    sprite.setTextColor(TFT_WHITE, TFT_RED);
  }
  else {
    sprite.clear(TFT_BLACK);
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  }

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
void createNewRecord(JsonObject &obj, int id, char *uid) {
  // 追加する要素を作成

  char timeStr[64];
  getLocalTime(&timeInfo);

  char buf[4];
  sprintf(buf, "%03d", id);
  obj["id"] = buf;
  obj["uid"] = uid;

  sprintf(timeStr, "%04d/%02d/%02d %02d:%02d:%02d", timeInfo.tm_year + 1900,
          timeInfo.tm_mon + 1, timeInfo.tm_mday, timeInfo.tm_hour,
          timeInfo.tm_min, timeInfo.tm_sec);
  Serial.printf("time:%s\n", timeStr);
  obj["scandate"] = timeStr;

  time_t expireDate = mktime(&timeInfo);

  expireDate += data.minute * 1; // テストモードで分→秒
  expireDate += data.hour * 3600;
  expireDate += data.day * 24 * 60 * 60;

  sprintf(timeStr, "%ld", expireDate);
  obj["expire"] = timeStr;
}

// JSONの内容を画面に表示する
// messageは先頭に表示される
void displayJSON(JsonObject &object, char *message) {
  const char *id = object["id"];
  const char *uid = object["uid"];
  const char *scandate = object["scandate"];
  const char *expire = object["expire"];
  const char *unixTime = object["expire"];

  long lUnixTime;
  sscanf(unixTime, "%ld", &lUnixTime);
  struct tm *timeInfo = localtime((time_t *)&lUnixTime);
  char timeStr[64];
  sprintf(timeStr, "%04d/%02d/%02d %02d:%02d:%02d", timeInfo->tm_year + 1900,
          timeInfo->tm_mon + 1, timeInfo->tm_mday, timeInfo->tm_hour,
          timeInfo->tm_min, timeInfo->tm_sec);
  Serial.printf("expire time:%s\n", timeStr);

  Serial.printf("%s id:%s uid:%s scandate:%s expire:%s\n", message, id, uid,
                scandate, expire);
  sprintf(data.message, "%s\nid:%s\nuid:%s\nscandate:%s\nexpire:%s\n", message,
          id, uid, scandate, timeStr);
}

void setup() {
  M5.begin(true, true, true, true);
  M5.Power.begin();
  delay(500);

  // SDチェック
  if (SD.exists("/data.json")) {
    data.isSdEnable = true;
    Serial.printf("SD Card Found\n");
  }
  else {
    // ファイルが存在しなければ作成を試みる
    // SDが存在していれば次のリセット時にチェックが通る
    File f = SD.open("/data.json", FILE_WRITE);
    f.close();
    Serial.printf("SD Card Not Found\n");
  }

  mfrc522.PCD_Init();

  lcd.init();
  lcd.setRotation(1);

  lcd.setBrightness(128);

  sprite.setPsram(true);
  sprite.setColorDepth(8);
  sprite.createSprite(320, 240);
  makeSprite();

  if (data.isSdEnable) {
    // JSONチェック
    Serial.print("setup json check.\n");

    // SDカードから読み込む
    File f = SD.open("/data.json");

    DynamicJsonDocument jsonDocument(1024);
    // deseriarizeする
    DeserializationError error = deserializeJson(jsonDocument, f);
    f.close();
    f = SD.open("/data.json");

    if (error) {
      // data.jsonが空
      Serial.print("data.json is empty.\n");
      Serial.println(error.c_str());
    }
    else {
      // deseriarize成功
      Serial.print("deserialized:\n");
      serializeJsonPretty(jsonDocument, Serial);
      Serial.println();

      if (jsonDocument.isNull()) {
        Serial.print("data.json is null.\n");
        f = SD.open("/data.json", FILE_WRITE);
        // fileクリア
        f.close();
        Serial.println("data.json cleared");
      }
    }
    f.close();
  }

  WiFi.begin();

  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // 500ms毎に.を表示
    Serial.print(".");
    count++;
    if (count == 10) {
      // 5秒間待ってからWPSを開始する
      // 一度WiFiを切断してwpsConnect()を使う
      WiFi.disconnect();
      wpsConnect();
    }
  }
  if (WiFi.status()) {
    Serial.println("\nConnected");
    Serial.println("ntp configured");

    configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com",
               "ntp.jst.mfeed.ad.jp");
    data.isWifiEnable = true;
  }

  data.time = millis();
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

  if (!data.isSdEnable) {
    // SDが使えなかったらreturn
    return;
  }

  // ここから期限切れチェック
  unsigned long now = millis();

  if (10000 <= now - data.time) {
    Serial.printf("10sec %ld\n", now / 1000);
    data.time = now;
    File f = SD.open("/data.json");
    DynamicJsonDocument jsonDocument(1024);
    DeserializationError error = deserializeJson(jsonDocument, f);
    f.close();
    if (!error) {
      // JSONデータが存在する
      JsonArray array = jsonDocument["json"].as<JsonArray>();
      for (int i = 0; i < array.size(); i++) {
        JsonObject object = array[i];

        const char *unixTime = object["expire"];

        Serial.printf("expire char:%s\n", unixTime);
        getLocalTime(&timeInfo);
        time_t nowTime = mktime(&timeInfo);
        Serial.printf("nowTime long:%ld\n", nowTime);

        long lUnixTime;
        sscanf(unixTime, "%ld", &lUnixTime);
        // 期限が過去ならば
        if (lUnixTime < nowTime) {
          // TODO displayJSON(JsonObject object, char * message)を作る
          // 期限切れ表示
          displayJSON(object, "期限が切れています");
          data.isExpired = true;
          makeSprite();
          break;
        }
      }
    }
    data.isExpired = false;
  }

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    // RFIDカードが読めていないとreturn
    delay(50);
    return;
  }
  mfrc522.PCD_AntennaOff();
  data.uidSize = mfrc522.uid.size;
  char buf[9];

  sprintf(buf, "%02X%02X%02X%02X", mfrc522.uid.uidByte[0],
          mfrc522.uid.uidByte[1], mfrc522.uid.uidByte[2],
          mfrc522.uid.uidByte[3]);
  // 読み込んだuidを構造体に入れる
  data.uidChar = buf;
  Serial.printf("RFID read\n");
  sprite.clear(TFT_WHITE);
  sprite.pushSprite(&lcd, 0, 0);
  delay(3000);
  mfrc522.PCD_AntennaOn();
  if (!data.isSdEnable) {
    Serial.printf("SD NG\n");
    return;
  }
  // SDカードからJSONを読み込む
  File f = SD.open("/data.json");
  if (!f) {
    Serial.printf("/data.json Not Found\n");
    delay(1000);
    return;
  }

  DynamicJsonDocument jsonDocument(1024);
  DeserializationError error = deserializeJson(jsonDocument, f);
  // deserializeするとFileに書き込めなくなるので開きなおす
  f.close();

  if (error) {
    // 追加モードの時
    if (data.mode == 0) {
      f = SD.open("/data.json", FILE_WRITE);

      // data.jsonが空
      jsonDocument.clear();
      Serial.print("data.json is empty.\n");
      Serial.println(error.c_str());
      // json配列を作成
      JsonArray array = jsonDocument.createNestedArray("json");
      JsonObject object = array.createNestedObject();
      createNewRecord(object, 0, data.uidChar);
      // // json配列に要素を追加
      // jsonDocument.add(array);

      // SDカードに書きこむ
      size_t size = serializeJsonPretty(jsonDocument, Serial);
      Serial.printf("\nSerialize size:%d\n", size);
      size = serializeJsonPretty(jsonDocument, f);
      Serial.printf("Serialize size:%d\n", size);
      displayJSON(object, "追加されました");
      Serial.print("JSON Wrote to SD Card\n");
      makeSprite();
      delay(1000);
      f.close();
      return;
    }
  }

  // data.jsonにデータがある
  Serial.println("JSON data found\n");
  JsonArray array = jsonDocument["json"].as<JsonArray>();
  for (int i = 0; i < array.size(); i++) {
    JsonObject object = array[i];
    const char *uid = object["uid"];
    // Serial.printf("new uid:%s\n", data.uidChar);
    if (uid == nullptr) {
      continue;
    }

    // Serial.printf("json uid:%s\n", uid);

    if (!strcmp(uid, data.uidChar)) {
      // スキャンされたuidが既に存在する
      Serial.printf("UID :%s found in json.\n", data.uidChar);
      switch (data.mode) {
      case 0:
        // 画面を更新して終了
        Serial.printf("UID :%s already exist.\n", data.uidChar);
        displayJSON(object, "既に存在します");
        break;
      case 1: {
        f = SD.open("/data.json", FILE_WRITE);
        // 削除
        array.remove(i);
        Serial.println("Matched record removed.");
        displayJSON(object, "削除されました");
        // jsonを表示
        serializeJsonPretty(jsonDocument, Serial);
        Serial.println();

        if (array.size() == 0) {
          // 何もせずに閉じると空ファイルになる
          f.close();
          Serial.println("data.json cleared");
          makeSprite();
          return;
        }
        // SDカードに書きこむ
        size_t size = serializeJsonPretty(jsonDocument, f);
        Serial.printf("Serialize size:%d\n", size);
        delay(1000);
        f.close();
        Serial.print("JSON Wrote to SD Card\n");
        displayJSON(object, "削除されました");
        break;
      }
      case 2:
        // 確認
        displayJSON(object, "登録されています");

        break;

      default:
        break;
      }
      makeSprite();
      return;
    }
  }
  // データが見つからなかった場合
  switch (data.mode) {
  case 0:
    // 追加
    // データが見つからなかったのでIDを増やして追加
    {
      f = SD.open("/data.json", FILE_WRITE);
      // 一番古いidを取得
      int newestId = searchNewestId(jsonDocument);
      Serial.printf("newestId:%d\n", newestId);

      // 追加する要素を作成

      // rootを"json"とする配列を取得
      JsonArray jsonArray = jsonDocument["json"].as<JsonArray>();
      JsonObject obj = jsonArray.createNestedObject();
      // データを追加
      createNewRecord(obj, newestId + 1, data.uidChar);
      // jsonを表示
      serializeJsonPretty(jsonDocument, Serial);
      Serial.println();
      // SDカードに書きこむ
      serializeJsonPretty(jsonDocument, f);
      Serial.print("JSON Wrote to SD Card\n");
      displayJSON(obj, "追加されました");
      delay(1000);
      f.close();
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
  makeSprite();
}