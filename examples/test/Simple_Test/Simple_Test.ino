/*
 * CH224A Simple Test
 * 
 * CH224Aの基本動作をテストするシンプルなスケッチ
 * 5秒ごとに電圧を切り替えます: 5V -> 9V -> 12V -> 15V -> 20V -> 5V...
 */
#include <M5Unified.h>
#include <Wire.h>

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

// 電圧リスト
const int voltages[] = {5, 9, 12, 15, 20, 28};
const int voltageCount = 6;
int currentIndex = 0;

unsigned long lastChangeTime = 0;
const unsigned long changeInterval = 2000;  // 2秒

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
  M5.Display.fillRect(0, 0, 320, 40, NAVY);
  M5.Display.drawCentreString("CH224A Test", 160, 5, 2);
  
  // 初期電圧設定
  setVoltage(VOLTAGE_5V);
  updateDisplay();

  // 出力ON
  digitalWrite(VBUSEN_O, HIGH);
}

void loop() {
  M5.update();
  
  unsigned long currentTime = millis();
  
  // 2秒ごとに電圧を切り替え
  if (currentTime - lastChangeTime >= changeInterval) {
    currentIndex = (currentIndex + 1) % voltageCount;
    setVoltage(currentIndex);
    updateDisplay();
    lastChangeTime = currentTime;
  }
  
  // 残り時間を表示
  unsigned long remainingTime = changeInterval - (currentTime - lastChangeTime);
  int seconds = remainingTime / 1000;
  
  M5.Display.fillRect(0, 190, 320, 50, BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(CYAN);
  String timeStr = "Next change in: " + String(seconds) + "s";
  M5.Display.drawCentreString(timeStr, 160, 200, 2);
  
  delay(100);
}

// 電圧設定
void setVoltage(int mode) {
  Wire.beginTransmission(CH224A_ADDR);
  Wire.write(REG_VOLTAGE_CTRL);
  Wire.write(mode);
  Wire.endTransmission();
  delay(100);
}

// 画面更新
void updateDisplay() {
  M5.Display.fillRect(0, 80, 320, 80, BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(GREEN);
  String voltageStr = String(voltages[currentIndex]) + "V";
  M5.Display.drawCentreString(voltageStr, 160, 100, 4);
}
