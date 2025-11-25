/*
 * M5Stack Core Basic - CH224A Source Capabilities Reader
 * 
 * CH224Aチップのレジスタ0x60〜0x8FからSRCCAP（Source_Capabilities Message）
 * およびEPR_SRCCAP（EPR_Source_Capabilities Message）を取得して表示します。
 * 
 * 接続:
 * - M5Stack SDA (GPIO21) -> CH224A CFG3/SDA (Pin 3)
 * - M5Stack SCL (GPIO22) -> CH224A CFG2/SCL (Pin 2)
 * - M5Stack GND -> CH224A GND (Pin 0)
 * 
 * ボタン操作:
 * - ボタンA: SRCCAPデータを更新
 * - ボタンB: 表示モード切替(SRCCAP/EPR_SRCCAP)
 * - ボタンC: 画面をクリアして再表示
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
#define REG_PD_DATA_START  0x60  // PD電源データ開始アドレス
#define REG_PD_DATA_END    0x8F  // PD電源データ終了アドレス

// 表示モード
enum DisplayMode {
  DISPLAY_SRCAP = 0,
  DISPLAY_EPR_SRCAP = 1
};

// グローバル変数
DisplayMode currentMode = DISPLAY_SRCAP;
uint8_t pdData[48];  // 0x60-0x8Fのデータを格納（48バイト）
bool dataValid = false;

// 関数プロトタイプ
bool readPDData();
void displaySRCCAP();
void displayEPRSRCCAP();
void displayPDODetails(uint8_t* data, int startIdx, int pdoCount);
void parseFixedPDO(uint32_t pdo, int pdoNum);
void parseBatteryPDO(uint32_t pdo, int pdoNum);
void parseVariablePDO(uint32_t pdo, int pdoNum);
void parsePPSPDO(uint32_t pdo, int pdoNum);
void updateDisplay();
void drawHeader();
void drawFooter();

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = false;
  M5.begin(cfg);
  
  // I2C初期化
  Wire.begin(sdaPin, sclPin);
  
  // 画面初期化
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(WHITE);
  
  // タイトル表示
  drawHeader();
  
  // 初期データ読み込み
  delay(100);
  if (readPDData()) {
    dataValid = true;
    updateDisplay();
  } else {
    M5.Display.setTextColor(RED);
    M5.Display.setTextSize(2);
    M5.Display.drawCentreString("Failed to read CH224A!", 160, 120, 2);
  }
  
  drawFooter();
}

void loop() {
  M5.update();
  
  // ボタンA: SRCCAPデータを更新
  if (M5.BtnA.wasPressed()) {
    if (readPDData()) {
      dataValid = true;
      updateDisplay();
    } else {
      M5.Display.setTextColor(RED);
      M5.Display.setTextSize(1);
      M5.Display.drawString("Read failed!", 10, 60, 2);
      delay(1000);
      updateDisplay();
    }
  }
  
  // ボタンB: 表示モード切替
  if (M5.BtnB.wasPressed()) {
    currentMode = (currentMode == DISPLAY_SRCAP) ? DISPLAY_EPR_SRCAP : DISPLAY_SRCAP;
    updateDisplay();
  }
  
  // ボタンC: 画面をクリアして再表示
  if (M5.BtnC.wasPressed()) {
    M5.Display.fillScreen(BLACK);
    drawHeader();
    if (dataValid) {
      updateDisplay();
    }
    drawFooter();
  }
  
  delay(10);
}

// CH224AのPDデータレジスタ(0x60-0x8F)を読み込み
bool readPDData() {
  Wire.beginTransmission(CH224A_ADDR);
  Wire.write(REG_PD_DATA_START);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  
  // 48バイトのデータを要求
  uint8_t bytesReceived = Wire.requestFrom(CH224A_ADDR, (uint8_t)48);
  
  if (bytesReceived == 48) {
    for (int i = 0; i < 48; i++) {
      pdData[i] = Wire.read();
//      M5.Display.printf("0x%02x", pdData[i]);
    }
    return true;
  }
  
  return false;
}

// SRCCAPデータを表示
void displaySRCCAP() {
  M5.Display.setCursor(0, 40);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(CYAN);
  M5.Display.println("SRCCAP Analysis");
  
  if (!dataValid) {
    M5.Display.setTextColor(RED);
    M5.Display.println("No data available");
    return;
  }
  
  // PDO詳細表示
  displayPDODetails(7);
}

// EPR_SRCCAPデータを表示
void displayEPRSRCCAP() {
  M5.Display.setTextColor(CYAN);
  M5.Display.setTextSize(2);
  M5.Display.drawString("EPR_SRCCAP Analysis:", 10, 60, 2);
  
  if (!dataValid) {
    M5.Display.setTextColor(RED);
    M5.Display.setTextSize(1);
    M5.Display.drawString("No data available", 10, 85, 2);
    return;
  }
  
  // EPRデータは0x80-0x8Fに格納（オフセット32から）
  uint8_t eprPdoCount = pdData[32];
  if (eprPdoCount == 0) {
    M5.Display.setTextColor(RED);
    M5.Display.setTextSize(1);
    M5.Display.drawString("EPR Not Supported", 10, 85, 2);
    return;
  }
  
  // EPR PDO詳細表示
  displayPDODetails(eprPdoCount);
}

// PDO詳細情報を表示
void displayPDODetails(int pdoCount) {
  uint32_t pdo[pdoCount];
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);

  // PDO数表示
  M5.Display.setTextColor(GREEN);
  String countStr = "PDO Count: " + String(pdoCount);
  M5.Display.println(countStr);
  // 各PDOを解析して表示
  for (int i = 0; i < pdoCount; i++) {
    // PDOは4バイト（32ビット）
    pdo[i] = ((uint32_t)pdData[i*4 + 0] << 24) |
             ((uint32_t)pdData[i*4 + 1] << 16) |
             ((uint32_t)pdData[i*4 + 2] << 8) |
             (uint32_t)pdData[i*4 + 3];

    M5.Display.setTextColor(YELLOW);
    M5.Display.print("PDO " + String(i + 1) + ": ");
    M5.Display.setTextColor(CYAN);
    M5.Display.printf("0x%08x\n\r", pdo[i]);

    // PDOタイプに応じて解析
    uint8_t pdoType = (pdo[i] >> 30) & 0x03;
    
    switch (pdoType) {
      case 0: // Fixed Supply
        parseFixedPDO(pdo[i]);
        break;
      case 1: // Variable Supply
        parseVariablePDO(pdo[i]);
        break;
      case 2: // Battery Supply
        parseFixedPDO(pdo[i]);
        break;
      case 3: // Programmable Supply
        parseVariablePDO(pdo[i]);
        break;
    }
  }

}

// 固定電源PDOを解析
void parseFixedPDO(uint32_t pdo) {
  M5.Display.setTextColor(CYAN);
  M5.Display.print(" Fixed Voltage, ");
  
  // 電圧 (50mV単位)
  uint16_t voltage = (pdo >> 10) & 0x3FF;
  float voltageV = voltage * 0.05;
  
  // 電流 (10mA単位)
  uint16_t current = (pdo >> 0) & 0x3FF;
  float currentA = current * 0.01;
  
  M5.Display.setTextColor(WHITE);
  String voltageStr = "Voltage: " + String(voltageV, 1) + "V";
  String currentStr = "Current: " + String(currentA, 2) + "A";
  
  M5.Display.print(voltageStr + ", ");
  M5.Display.println(currentStr);
}

// 可変電源PDOを解析
void parseVariablePDO(uint32_t pdo) {
  M5.Display.setTextColor(CYAN);
  M5.Display.print(" Variable Voltage, ");
  
  // 最小電圧 (100mV単位)
  uint16_t minVoltage = (pdo >> 8) & 0xFF;
  float minVoltageV = minVoltage * 0.1;
  
  // 最大電圧 (100mV単位)
  uint16_t maxVoltage = (pdo >> 17) & 0xFF;
  float maxVoltageV = maxVoltage * 0.1;
  
  // 最大電流 (50mA単位)
  uint16_t maxCurrent = (pdo >> 0) & 0x7F;
  float maxCurrentA = maxCurrent * 0.05;
  
  M5.Display.setTextColor(WHITE);
  String voltageStr = String(minVoltageV, 1) + "V - " + String(maxVoltageV, 1) + "V";
  String currentStr = "Imax: " + String(maxCurrentA, 2) + "A";
  
  M5.Display.print(voltageStr + ", ");
  M5.Display.println(currentStr);
}

// PPS PDOを解析
void parsePPSPDO(uint32_t pdo, int yPos) {
  M5.Display.setTextColor(CYAN);
  M5.Display.drawString("Type: PPS Supply", 20, yPos, 2);
  
  // 最小電圧 (20mV単位)
  uint16_t minVoltage = (pdo >> 8) & 0xFF;
  float minVoltageV = minVoltage * 0.02;
  
  // 最大電圧 (20mV単位)
  uint16_t maxVoltage = (pdo >> 16) & 0xFF;
  float maxVoltageV = maxVoltage * 0.02;
  
  // 最大電流 (50mA単位)
  uint16_t maxCurrent = (pdo >> 0) & 0xFF;
  float maxCurrentA = maxCurrent * 0.05;
  
  M5.Display.setTextColor(WHITE);
  String voltageStr = String(minVoltageV, 1) + "V - " + String(maxVoltageV, 1) + "V";
  String currentStr = "Max Current: " + String(maxCurrentA, 2) + "A";
  String powerStr = "Max Power: " + String(maxVoltageV * maxCurrentA, 1) + "W";
  
  M5.Display.drawString(voltageStr, 20, yPos + 10, 2);
  M5.Display.drawString(currentStr, 20, yPos + 20, 2);
  M5.Display.drawString(powerStr, 160, yPos + 20, 2);
  
  // PPSリアルタイム制御サポート
  bool realTimeControl = (pdo >> 24) & 0x01;
  if (realTimeControl) {
    M5.Display.setTextColor(GREEN);
    M5.Display.drawString("Real-time Control", 160, yPos + 10, 2);
  }
}

// 画面更新
void updateDisplay() {
  // メイン表示エリアをクリア
  M5.Display.fillRect(0, 40, 320, 180, BLACK);
  
  // 現在のモードに応じてデータを表示
  if (currentMode == DISPLAY_SRCAP) {
    displaySRCCAP();
  } else {
    displayEPRSRCCAP();
  }
  
  drawFooter();
}

// ヘッダー描画
void drawHeader() {
  M5.Display.fillRect(0, 0, 320, 25, NAVY);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);
  M5.Display.drawCentreString("CH224A Source Capabilities", 160, 3, 2);
  
  M5.Display.setTextSize(1);
  String modeStr = (currentMode == DISPLAY_SRCAP) ? "Mode: SRCCAP" : "Mode: EPR_SRCCAP";
  M5.Display.setTextColor(YELLOW);
  M5.Display.drawString(modeStr, 10, 25, 2);
}

// フッター描画
void drawFooter() {
  M5.Display.fillRect(0, 220, 320, 20, BLUE);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);
  
  M5.Display.drawString("READ", 25, 220, 2);
  M5.Display.drawString("MODE", 135, 220, 2);
  M5.Display.drawString("CLEAR", 230, 220, 2);

}
