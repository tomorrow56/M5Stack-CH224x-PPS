/*
 * M5Stack Core Basic - CH224A PPS Mode Controller (Library Version)
 * 
 * CH224Aライブラリを使用した改良版スケッチ
 * 
 * 接続:
 * - M5Stack SDA (GPIO21) -> CH224A CFG3/SDA (Pin 3)
 * - M5Stack SCL (GPIO22) -> CH224A CFG2/SCL (Pin 2)
 * - M5Stack GND -> CH224A GND (Pin 0)
 * 
 * ボタン操作:
 * - ボタンA: 電圧を下げる
 * - ボタンB: モード切替(固定電圧/PPSモード)
 * - ボタンC: 電圧を上げる
 */

#include <M5Stack.h>
#include "CH224A.h"

// CH224Aオブジェクト作成
CH224A ch224a(CH224A_I2C_ADDR_DEFAULT);

// 固定電圧プリセット
const float fixedVoltages[] = {5.0, 9.0, 12.0, 15.0, 20.0};
const int fixedVoltageCount = 5;

// グローバル変数
bool ppsMode = false;           // PPSモード有効/無効
int currentFixedIndex = 0;      // 現在の固定電圧インデックス
float currentPPSVoltage = 9.0;  // 現在のPPS電圧(V)
float minPPSVoltage = 5.0;      // 最小PPS電圧(V)
float maxPPSVoltage = 21.0;     // 最大PPS電圧(V)
float ppsStep = 0.2;            // PPS電圧ステップ(V)

// 関数プロトタイプ
void updateDisplay();
void drawButton(int x, int y, int w, int h, const char* label, uint16_t color);
void showErrorMessage(const char* message);

void setup() {
  // M5Stack初期化
  M5.begin();
  M5.Power.begin();
  
  // I2C初期化
  Wire.begin(21, 22);  // SDA=21, SCL=22
  
  // 画面初期化
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  
  // タイトル表示
  M5.Lcd.fillRect(0, 0, 320, 40, NAVY);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.drawCentreString("CH224A PPS Controller", 160, 12, 2);
  
  // CH224A初期化
  M5.Lcd.drawCentreString("Initializing CH224A...", 160, 100, 2);
  delay(500);
  
  if (!ch224a.begin(Wire)) {
    showErrorMessage("CH224A not found!");
    while (1) {
      delay(1000);
    }
  }
  
  // 初期電圧設定(9V)
  if (!ch224a.setVoltage9V()) {
    showErrorMessage("Failed to set voltage!");
    delay(2000);
  }
  currentFixedIndex = 1;
  
  // 画面更新
  updateDisplay();
}

void loop() {
  M5.update();
  
  // ボタンA: 電圧を下げる
  if (M5.BtnA.wasPressed()) {
    if (ppsMode) {
      // PPSモード: 電圧を下げる
      currentPPSVoltage -= ppsStep;
      if (currentPPSVoltage < minPPSVoltage) {
        currentPPSVoltage = minPPSVoltage;
      }
      if (!ch224a.setPPSVoltage(currentPPSVoltage)) {
        showErrorMessage("PPS set failed!");
        delay(1000);
      }
    } else {
      // 固定電圧モード: 前の電圧に切り替え
      currentFixedIndex--;
      if (currentFixedIndex < 0) {
        currentFixedIndex = 0;
      }
      
      // 電圧設定
      bool success = false;
      switch (currentFixedIndex) {
        case 0: success = ch224a.setVoltage5V(); break;
        case 1: success = ch224a.setVoltage9V(); break;
        case 2: success = ch224a.setVoltage12V(); break;
        case 3: success = ch224a.setVoltage15V(); break;
        case 4: success = ch224a.setVoltage20V(); break;
      }
      
      if (!success) {
        showErrorMessage("Voltage set failed!");
        delay(1000);
      }
    }
    updateDisplay();
  }
  
  // ボタンB: モード切替
  if (M5.BtnB.wasPressed()) {
    ppsMode = !ppsMode;
    
    bool success = false;
    if (ppsMode) {
      // PPSモードに切り替え
      success = ch224a.setPPSVoltage(currentPPSVoltage);
    } else {
      // 固定電圧モードに切り替え
      switch (currentFixedIndex) {
        case 0: success = ch224a.setVoltage5V(); break;
        case 1: success = ch224a.setVoltage9V(); break;
        case 2: success = ch224a.setVoltage12V(); break;
        case 3: success = ch224a.setVoltage15V(); break;
        case 4: success = ch224a.setVoltage20V(); break;
      }
    }
    
    if (!success) {
      showErrorMessage("Mode switch failed!");
      delay(1000);
    }
    
    updateDisplay();
  }
  
  // ボタンC: 電圧を上げる
  if (M5.BtnC.wasPressed()) {
    if (ppsMode) {
      // PPSモード: 電圧を上げる
      currentPPSVoltage += ppsStep;
      if (currentPPSVoltage > maxPPSVoltage) {
        currentPPSVoltage = maxPPSVoltage;
      }
      if (!ch224a.setPPSVoltage(currentPPSVoltage)) {
        showErrorMessage("PPS set failed!");
        delay(1000);
      }
    } else {
      // 固定電圧モード: 次の電圧に切り替え
      currentFixedIndex++;
      if (currentFixedIndex >= fixedVoltageCount) {
        currentFixedIndex = fixedVoltageCount - 1;
      }
      
      // 電圧設定
      bool success = false;
      switch (currentFixedIndex) {
        case 0: success = ch224a.setVoltage5V(); break;
        case 1: success = ch224a.setVoltage9V(); break;
        case 2: success = ch224a.setVoltage12V(); break;
        case 3: success = ch224a.setVoltage15V(); break;
        case 4: success = ch224a.setVoltage20V(); break;
      }
      
      if (!success) {
        showErrorMessage("Voltage set failed!");
        delay(1000);
      }
    }
    updateDisplay();
  }
  
  delay(10);
}

// 画面更新
void updateDisplay() {
  // メイン表示エリアをクリア
  M5.Lcd.fillRect(0, 40, 320, 180, BLACK);
  
  // モード表示
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setTextSize(2);
  if (ppsMode) {
    M5.Lcd.drawCentreString("Mode: PPS", 160, 50, 2);
  } else {
    M5.Lcd.drawCentreString("Mode: Fixed Voltage", 160, 50, 2);
  }
  
  // 電圧表示
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(4);
  String voltageStr;
  if (ppsMode) {
    voltageStr = String(currentPPSVoltage, 1) + "V";
  } else {
    voltageStr = String(fixedVoltages[currentFixedIndex], 1) + "V";
  }
  M5.Lcd.drawCentreString(voltageStr, 160, 100, 4);
  
  // 電圧範囲表示(PPSモードのみ)
  if (ppsMode) {
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.setTextSize(1);
    String rangeStr = "Range: " + String(minPPSVoltage, 1) + "V - " + String(maxPPSVoltage, 1) + "V";
    M5.Lcd.drawCentreString(rangeStr, 160, 160, 2);
    
    String stepStr = "Step: " + String(ppsStep, 1) + "V";
    M5.Lcd.drawCentreString(stepStr, 160, 180, 2);
  }
  
  // ボタンラベル表示
  drawButton(10, 220, 90, 20, "DOWN", BLUE);
  drawButton(115, 220, 90, 20, "MODE", ORANGE);
  drawButton(220, 220, 90, 20, "UP", BLUE);
}

// ボタンラベル描画
void drawButton(int x, int y, int w, int h, const char* label, uint16_t color) {
  M5.Lcd.fillRect(x, y, w, h, color);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);
  int textX = x + w / 2;
  int textY = y + h / 2 - 8;
  M5.Lcd.drawCentreString(label, textX, textY, 2);
}

// エラーメッセージ表示
void showErrorMessage(const char* message) {
  M5.Lcd.fillRect(0, 90, 320, 60, RED);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawCentreString(message, 160, 110, 2);
}
