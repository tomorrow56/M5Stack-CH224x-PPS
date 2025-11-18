/*
 * I2C Scanner for M5Stack
 * 
 * I2Cバス上のデバイスをスキャンしてアドレスを表示します。
 * CH224Aのアドレスを確認するために使用できます。
 */

#include <M5Unified.h>
#include <Wire.h>

TwoWire cfgWire = TwoWire(1);

// M5Stack Pin config
// no PSRAM model only 
#define VBUS_I          35
#define VI_I            36
#define CFG2_O          22
#define CFG3_O          21
#define VBUSEN_O         2
#define PG_I            12

const int sdaPin = CFG3_O;
const int sclPin = CFG2_O;

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = false;
  M5.begin(cfg);
  

  // I2C初期化
  cfgWire.begin(sdaPin, sclPin, 100000);

  // 画面初期化
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(WHITE);
  
  // タイトル表示
  M5.Display.fillRect(0, 0, 320, 30, NAVY);
  M5.Display.drawCentreString("I2C Scanner", 160, 8, 2);
  
  M5.Display.setCursor(10, 35);
  M5.Display.println("Scanning I2C bus...");
  
  delay(1000);

  // I2Cスキャン
  int deviceCount = 0;
  int yPos = 50;

  for (byte address = 1; address < 127; address++) {
    cfgWire.beginTransmission(address);
    byte error = cfgWire.endTransmission();
    
    if (error == 0) {
      deviceCount++;
      
      M5.Display.setCursor(10, yPos);
      M5.Display.print("Device found: 0x");
      if (address < 16) {
        M5.Display.print("0");
      }
      M5.Display.print(address, HEX);
      
      // CH224Aかどうか判定
      if (address == 0x22 || address == 0x23) {
        M5.Display.setTextColor(GREEN);
//        M5.Display.setCursor(30, yPos + 10);
        M5.Display.println("(CH224A detected!)");
        M5.Display.setTextColor(WHITE);
        yPos += 5;
      }

      // IP5306かどうか判定
      if (address == 0x75) {
        M5.Display.setTextColor(GREEN);
//        M5.Display.setCursor(30, yPos + 10);
        M5.Display.println("(IP5306 detected!)");
        M5.Display.setTextColor(WHITE);
        yPos += 5;
      }

      yPos += 10;
      
      if (yPos > 200) {
        break;  // 画面に収まらない場合は中断
      }
    }
  }
  
  // 結果表示
  M5.Display.setCursor(10, yPos + 10);
  if (deviceCount == 0) {
    M5.Display.setTextColor(RED);
    M5.Display.println("No I2C devices found!");
  } else {
    M5.Display.setTextColor(GREEN);
    M5.Display.print("Found ");
    M5.Display.print(deviceCount);
    M5.Display.println(" device(s)");
  }

  M5.Display.setTextColor(WHITE);
  M5.Display.setCursor(10, 220);
  M5.Display.println("Press any button to rescan");
}

void loop() {
  M5.update();
  
  // いずれかのボタンが押されたら再スキャン
  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
    setup();
  }
  
  delay(100);
}
