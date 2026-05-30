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

#include <M5Unified.h>
#include <Wire.h>

// M5Stack Pin config
#define VBUS_I          35
#define VI_I            36
#define CFG2_O          22
#define CFG3_O          21
#define VBUSEN_O         2
#define PG_I            12

const int sdaPin = CFG3_O;
const int sclPin = CFG2_O;

// CH224A I2Cアドレス
#define CH224A_ADDR 0x22

// CH224Aレジスタアドレス
#define REG_I2C_STATUS    0x09
#define REG_VOLTAGE_CTRL  0x0A
#define REG_CURRENT_MAX   0x50 //50mA step
#define REG_AVS_VOLTAGE_H 0x51 //100mV step
#define REG_AVS_VOLTAGE_L 0x52
#define REG_PPS_VOLTAGE   0x53 //100mV step
// 0x60-0x8F REG_SRCCAP_DATA_x


// 電圧モード
#define VOLTAGE_5V   0
#define VOLTAGE_9V   1
#define VOLTAGE_12V  2
#define VOLTAGE_15V  3
#define VOLTAGE_20V  4
#define VOLTAGE_28V  5
#define VOLTAGE_PPS  6
#define VOLTAGE_AVS  7

// 固定電圧プリセット
const int fixedVoltages[] = {5, 9, 12, 15, 20, 28};
const int fixedVoltageCount = 6;

// グローバル変数
bool ppsMode = false;           // PPSモード有効/無効
int currentFixedIndex = 0;      // 現在の固定電圧インデックス
int currentPPSVoltage = 55;     // 現在のPPS電圧(0.1V単位、5.5V)
int minPPSVoltage = 55;         // 最小PPS電圧(5.5V)
int maxPPSVoltage = 200;        // 最大PPS電圧(20.0V)
int ppsStep = 1;                // PPS電圧ステップ(0.1V)

// 関数プロトタイプ
void writeRegister(uint8_t reg, uint8_t value);
void setFixedVoltage(int voltageMode);
void setPPSVoltage(int voltage);
void updateDisplay();
void drawButton(int x, int y, int w, int h, const char* label, uint16_t color);

void setup() {
  pinMode(VBUSEN_O, OUTPUT);
  digitalWrite(VBUSEN_O, LOW);

  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = false;
  M5.begin(cfg);
  
  // I2C初期化
  Wire.begin(sdaPin, sclPin);
  
  // 画面初期化
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(WHITE);
  
  // タイトル表示
  M5.Display.fillRect(0, 0, 320, 35, NAVY);
  M5.Display.setTextColor(WHITE);
  M5.Display.drawCentreString("CH224A PPS Controller", 160, 3, 2);
  
  // 初期電圧設定(5V)
  delay(100);
  setFixedVoltage(VOLTAGE_5V);
  currentFixedIndex = 0;
  updateDisplay();

  // 出力ON
  digitalWrite(VBUSEN_O, HIGH);

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
  M5.Display.fillRect(0, 40, 320, 180, BLACK);
  
  // モード表示
  M5.Display.setTextColor(YELLOW);
  M5.Display.setTextSize(2);
  if (ppsMode) {
    M5.Display.drawCentreString("Mode: PPS", 160, 50, 2);
  } else {
    M5.Display.drawCentreString("Mode: Fixed Voltage", 160, 50, 2);
  }
  
  // 電圧表示
  M5.Display.setTextColor(GREEN);
  M5.Display.setTextSize(2);
  String voltageStr;
  if (ppsMode) {
    float voltage = currentPPSVoltage / 10.0;
    voltageStr = String(voltage, 1) + "V";
  } else {
    voltageStr = String(fixedVoltages[currentFixedIndex]) + "V";
  }
  M5.Display.drawCentreString(voltageStr, 160, 100, 4);
  
  // 電圧範囲表示(PPSモードのみ)
  if (ppsMode) {
    M5.Display.setTextColor(CYAN);
    M5.Display.setTextSize(2);
    String rangeStr = "Range: " + String(minPPSVoltage / 10.0, 1) + "V - " + String(maxPPSVoltage / 10.0, 1) + "V";
    M5.Display.drawCentreString(rangeStr, 160, 160, 2);
  }
  
  // ボタンラベル表示
  drawButton(10, 220, 90, 20, "DOWN", BLUE);
  drawButton(115, 220, 90, 20, "MODE", ORANGE);
  drawButton(220, 220, 90, 20, "UP", BLUE);
}

// ボタンラベル描画
void drawButton(int x, int y, int w, int h, const char* label, uint16_t color) {
  M5.Display.fillRect(x, y, w, h, color);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);
  int textX = x + w / 2;
  int textY = y + h / 2 - 8;
  M5.Display.drawCentreString(label, textX, textY, 2);
}
