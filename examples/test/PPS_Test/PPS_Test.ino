/*
 * CH224A PPS Test
 * 
 * PPSモードのテストスケッチ
 * 5.5Vから20.0Vまで0.5Vステップで電圧を上げていきます
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

// PPS電圧設定
float currentVoltage = 5.5;
float minVoltage = 5.5;
float maxVoltage = 20.0;
float voltageStep = 0.5;

unsigned long lastChangeTime = 0;
const unsigned long changeInterval = 1000;  // 1秒

bool increasing = true;

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
  M5.Display.drawCentreString("CH224A PPS Test", 160, 5, 2);
  
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(YELLOW);
  M5.Display.drawCentreString("Sweeping voltage", 160, 50, 2);
  
  // 初期電圧設定
  setPPSVoltage(VOLTAGE_5V);
  updateDisplay();

  // 出力ON
  digitalWrite(VBUSEN_O, HIGH);

}

void loop() {
  M5.update();
  
  unsigned long currentTime = millis();
  
  // 1秒ごとに電圧を変更
  if (currentTime - lastChangeTime >= changeInterval) {
    if (increasing) {
      currentVoltage += voltageStep;
      if (currentVoltage >= maxVoltage) {
        currentVoltage = maxVoltage;
        increasing = false;
      }
    } else {
      currentVoltage -= voltageStep;
      if (currentVoltage <= minVoltage) {
        currentVoltage = minVoltage;
        increasing = true;
      }
    }
    
    setPPSVoltage(currentVoltage);
    updateDisplay();
    lastChangeTime = currentTime;
  }
  
  // 方向表示
  M5.Display.fillRect(0, 180, 320, 30, BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(CYAN);
  String dirStr = increasing ? "Increasing..." : "Decreasing...";
  M5.Display.drawCentreString(dirStr, 160, 185, 2);
  
  delay(100);
}

// PPS電圧設定
void setPPSVoltage(float voltage) {
  // 電圧を0.1V単位に変換
  uint8_t voltageRaw = (uint8_t)(voltage * 10.0 + 0.5);
  
  // PPS電圧値を設定
  Wire.beginTransmission(CH224A_ADDR);
  Wire.write(REG_PPS_VOLTAGE);
  Wire.write(voltageRaw);
  Wire.endTransmission();
  delay(50);
  
  // PPSモードに切り替え
  Wire.beginTransmission(CH224A_ADDR);
  Wire.write(REG_VOLTAGE_CTRL);
  Wire.write(VOLTAGE_PPS);
  Wire.endTransmission();
  delay(50);
}

// 画面更新
void updateDisplay() {
  M5.Display.fillRect(0, 80, 320, 80, BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(GREEN);
  String voltageStr = String(currentVoltage, 1) + "V";
  M5.Display.drawCentreString(voltageStr, 160, 100, 4);
}
