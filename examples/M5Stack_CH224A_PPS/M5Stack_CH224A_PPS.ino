/*
 * M5Stack Core Basic - CH224A PPS Mode Controller
 * 
 * CH224AチップをI2C経由で制御し、USB PD PPSモードで電圧を調整します。
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
#include <Wire.h>

// CH224A I2Cアドレス
#define CH224A_ADDR 0x22

// CH224Aレジスタアドレス
#define REG_VOLTAGE_CTRL 0x0A  // 電圧制御レジスタ
#define REG_PPS_VOLTAGE  0x53  // PPS電圧設定レジスタ
#define REG_AVS_VOLTAGE_L 0x51 // AVS電圧設定レジスタ(低位)
#define REG_AVS_VOLTAGE_H 0x52 // AVS電圧設定レジスタ(高位)

// 電圧モード定義
#define VOLTAGE_5V   0
#define VOLTAGE_9V   1
#define VOLTAGE_12V  2
#define VOLTAGE_15V  3
#define VOLTAGE_20V  4
#define VOLTAGE_28V  5
#define VOLTAGE_PPS  6
#define VOLTAGE_AVS  7

// 固定電圧プリセット
const int fixedVoltages[] = {5, 9, 12, 15, 20};
const int fixedVoltageCount = 5;

// グローバル変数
bool ppsMode = false;           // PPSモード有効/無効
int currentFixedIndex = 0;      // 現在の固定電圧インデックス
int currentPPSVoltage = 90;     // 現在のPPS電圧(0.1V単位、9.0V)
int minPPSVoltage = 50;         // 最小PPS電圧(5.0V)
int maxPPSVoltage = 210;        // 最大PPS電圧(21.0V)
int ppsStep = 2;                // PPS電圧ステップ(0.2V)

// 関数プロトタイプ
void writeRegister(uint8_t reg, uint8_t value);
void setFixedVoltage(int voltageMode);
void setPPSVoltage(int voltage);
void updateDisplay();
void drawButton(int x, int y, int w, int h, const char* label, uint16_t color);

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
  
  // 初期電圧設定(9V)
  delay(100);
  setFixedVoltage(VOLTAGE_9V);
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
      setPPSVoltage(currentPPSVoltage);
    } else {
      // 固定電圧モード: 前の電圧に切り替え
      currentFixedIndex--;
      if (currentFixedIndex < 0) {
        currentFixedIndex = 0;
      }
      setFixedVoltage(currentFixedIndex);
    }
    updateDisplay();
  }
  
  // ボタンB: モード切替
  if (M5.BtnB.wasPressed()) {
    ppsMode = !ppsMode;
    if (ppsMode) {
      // PPSモードに切り替え
      setPPSVoltage(currentPPSVoltage);
    } else {
      // 固定電圧モードに切り替え
      setFixedVoltage(currentFixedIndex);
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
      setPPSVoltage(currentPPSVoltage);
    } else {
      // 固定電圧モード: 次の電圧に切り替え
      currentFixedIndex++;
      if (currentFixedIndex >= fixedVoltageCount) {
        currentFixedIndex = fixedVoltageCount - 1;
      }
      setFixedVoltage(currentFixedIndex);
    }
    updateDisplay();
  }
  
  delay(10);
}

// CH224Aレジスタに書き込み
void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(CH224A_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
  delay(50);  // 設定反映待ち
}

// 固定電圧を設定
void setFixedVoltage(int voltageMode) {
  writeRegister(REG_VOLTAGE_CTRL, voltageMode);
}

// PPS電圧を設定
void setPPSVoltage(int voltage) {
  // PPS電圧設定(0.1V単位)
  writeRegister(REG_PPS_VOLTAGE, voltage);
  // PPSモードに切り替え
  writeRegister(REG_VOLTAGE_CTRL, VOLTAGE_PPS);
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
    float voltage = currentPPSVoltage / 10.0;
    voltageStr = String(voltage, 1) + "V";
  } else {
    voltageStr = String(fixedVoltages[currentFixedIndex]) + "V";
  }
  M5.Lcd.drawCentreString(voltageStr, 160, 100, 4);
  
  // 電圧範囲表示(PPSモードのみ)
  if (ppsMode) {
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.setTextSize(1);
    String rangeStr = "Range: " + String(minPPSVoltage / 10.0, 1) + "V - " + String(maxPPSVoltage / 10.0, 1) + "V";
    M5.Lcd.drawCentreString(rangeStr, 160, 160, 2);
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
