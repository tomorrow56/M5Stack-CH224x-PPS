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

#include <M5Stack.h>
#include <Wire.h>

// CH224A I2Cアドレス
#define CH224A_ADDR 0x22

// CH224Aレジスタアドレス
#define REG_PD_DATA_START  0x60  // PD電源データ寄存器開始アドレス
#define REG_PD_DATA_END    0x8F  // PD電源データ寄存器終了アドレス

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
  drawHeader();
  
  // 初期データ読み込み
  delay(100);
  if (readPDData()) {
    dataValid = true;
    updateDisplay();
  } else {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawCentreString("Failed to read CH224A!", 160, 120, 2);
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
      M5.Lcd.setTextColor(RED);
      M5.Lcd.setTextSize(1);
      M5.Lcd.drawString("Read failed!", 10, 60, 2);
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
    M5.Lcd.fillScreen(BLACK);
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
    }
    return true;
  }
  
  return false;
}

// SRCCAPデータを表示
void displaySRCCAP() {
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawString("SRCCAP Analysis:", 10, 60, 2);
  
  if (!dataValid) {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("No data available", 10, 85, 2);
    return;
  }
  
  // PDO数を取得
  uint8_t pdoCount = pdData[0];
  if (pdoCount == 0 || pdoCount > 7) {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("Invalid PDO count: " + String(pdoCount), 10, 85, 2);
    return;
  }
  
  // PDO詳細表示
  displayPDODetails(pdData, 1, pdoCount);
}

// EPR_SRCCAPデータを表示
void displayEPRSRCCAP() {
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawString("EPR_SRCCAP Analysis:", 10, 60, 2);
  
  if (!dataValid) {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("No data available", 10, 85, 2);
    return;
  }
  
  // EPRデータは0x80-0x8Fに格納（オフセット32から）
  uint8_t eprPdoCount = pdData[32];
  if (eprPdoCount == 0) {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("EPR Not Supported", 10, 85, 2);
    return;
  }
  
  // EPR PDO詳細表示
  displayPDODetails(pdData, 33, eprPdoCount);
}

// PDO詳細情報を表示
void displayPDODetails(uint8_t* data, int startIdx, int pdoCount) {
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);
  
  int yPos = 85;
  
  // PDO数表示
  M5.Lcd.setTextColor(GREEN);
  String countStr = "PDO Count: " + String(pdoCount);
  M5.Lcd.drawString(countStr, 10, yPos, 2);
  yPos += 15;
  
  // 各PDOを解析して表示
  for (int i = 0; i < pdoCount && i < 7; i++) {
    // PDOは4バイト（32ビット）
    uint32_t pdo = ((uint32_t)data[startIdx + i*4] << 24) |
                   ((uint32_t)data[startIdx + i*4 + 1] << 16) |
                   ((uint32_t)data[startIdx + i*4 + 2] << 8) |
                   (uint32_t)data[startIdx + i*4 + 3];
    
    M5.Lcd.setTextColor(YELLOW);
    String pdoHeader = "PDO " + String(i + 1) + ":";
    M5.Lcd.drawString(pdoHeader, 10, yPos, 2);
    yPos += 12;
    
    // PDOタイプに応じて解析
    uint8_t pdoType = (pdo >> 30) & 0x03;
    
    switch (pdoType) {
      case 0: // Fixed Supply
        parseFixedPDO(pdo, yPos);
        break;
      case 1: // Battery Supply
        parseBatteryPDO(pdo, yPos);
        break;
      case 2: // Variable Supply
        parseVariablePDO(pdo, yPos);
        break;
      case 3: // Programmable Supply
        parsePPSPDO(pdo, yPos);
        break;
    }
    
    yPos += 35;
    if (yPos > 210) break; // 画面範囲チェック
  }
}

// 固定電源PDOを解析
void parseFixedPDO(uint32_t pdo, int yPos) {
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.drawString("Type: Fixed Supply", 20, yPos, 2);
  
  // 電圧 (50mV単位)
  uint16_t voltage = (pdo >> 10) & 0x3FF;
  float voltageV = voltage * 0.05;
  
  // 電流 (10mA単位)
  uint16_t current = (pdo >> 0) & 0x3FF;
  float currentA = current * 0.01;
  
  M5.Lcd.setTextColor(WHITE);
  String voltageStr = "Voltage: " + String(voltageV, 1) + "V";
  String currentStr = "Current: " + String(currentA, 2) + "A";
  String powerStr = "Power: " + String(voltageV * currentA, 1) + "W";
  
  M5.Lcd.drawString(voltageStr, 20, yPos + 10, 2);
  M5.Lcd.drawString(currentStr, 20, yPos + 20, 2);
  M5.Lcd.drawString(powerStr, 160, yPos + 20, 2);
  
  // ピーク電流サポート
  bool peakCurrent = (pdo >> 20) & 0x01;
  if (peakCurrent) {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.drawString("Peak Current", 160, yPos + 10, 2);
  }
}

// バッテリーPDOを解析
void parseBatteryPDO(uint32_t pdo, int yPos) {
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.drawString("Type: Battery Supply", 20, yPos, 2);
  
  // 最小電圧 (50mV単位)
  uint16_t minVoltage = (pdo >> 10) & 0x3FF;
  float minVoltageV = minVoltage * 0.05;
  
  // 最大電圧 (50mV単位)
  uint16_t maxVoltage = (pdo >> 20) & 0x3FF;
  float maxVoltageV = maxVoltage * 0.05;
  
  // 最大電流 (10mA単位)
  uint16_t maxCurrent = (pdo >> 0) & 0x3FF;
  float maxCurrentA = maxCurrent * 0.01;
  
  M5.Lcd.setTextColor(WHITE);
  String voltageStr = String(minVoltageV, 1) + "V - " + String(maxVoltageV, 1) + "V";
  String currentStr = "Max Current: " + String(maxCurrentA, 2) + "A";
  String powerStr = "Max Power: " + String(maxVoltageV * maxCurrentA, 1) + "W";
  
  M5.Lcd.drawString(voltageStr, 20, yPos + 10, 2);
  M5.Lcd.drawString(currentStr, 20, yPos + 20, 2);
  M5.Lcd.drawString(powerStr, 160, yPos + 20, 2);
}

// 可変電源PDOを解析
void parseVariablePDO(uint32_t pdo, int yPos) {
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.drawString("Type: Variable Supply", 20, yPos, 2);
  
  // 最小電圧 (50mV単位)
  uint16_t minVoltage = (pdo >> 10) & 0x3FF;
  float minVoltageV = minVoltage * 0.05;
  
  // 最大電圧 (50mV単位)
  uint16_t maxVoltage = (pdo >> 20) & 0x3FF;
  float maxVoltageV = maxVoltage * 0.05;
  
  // 最大電流 (10mA単位)
  uint16_t maxCurrent = (pdo >> 0) & 0x3FF;
  float maxCurrentA = maxCurrent * 0.01;
  
  M5.Lcd.setTextColor(WHITE);
  String voltageStr = String(minVoltageV, 1) + "V - " + String(maxVoltageV, 1) + "V";
  String currentStr = "Max Current: " + String(maxCurrentA, 2) + "A";
  String powerStr = "Max Power: " + String(maxVoltageV * maxCurrentA, 1) + "W";
  
  M5.Lcd.drawString(voltageStr, 20, yPos + 10, 2);
  M5.Lcd.drawString(currentStr, 20, yPos + 20, 2);
  M5.Lcd.drawString(powerStr, 160, yPos + 20, 2);
}

// PPS PDOを解析
void parsePPSPDO(uint32_t pdo, int yPos) {
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.drawString("Type: PPS Supply", 20, yPos, 2);
  
  // 最小電圧 (20mV単位)
  uint16_t minVoltage = (pdo >> 8) & 0xFF;
  float minVoltageV = minVoltage * 0.02;
  
  // 最大電圧 (20mV単位)
  uint16_t maxVoltage = (pdo >> 16) & 0xFF;
  float maxVoltageV = maxVoltage * 0.02;
  
  // 最大電流 (50mA単位)
  uint16_t maxCurrent = (pdo >> 0) & 0xFF;
  float maxCurrentA = maxCurrent * 0.05;
  
  M5.Lcd.setTextColor(WHITE);
  String voltageStr = String(minVoltageV, 1) + "V - " + String(maxVoltageV, 1) + "V";
  String currentStr = "Max Current: " + String(maxCurrentA, 2) + "A";
  String powerStr = "Max Power: " + String(maxVoltageV * maxCurrentA, 1) + "W";
  
  M5.Lcd.drawString(voltageStr, 20, yPos + 10, 2);
  M5.Lcd.drawString(currentStr, 20, yPos + 20, 2);
  M5.Lcd.drawString(powerStr, 160, yPos + 20, 2);
  
  // PPSリアルタイム制御サポート
  bool realTimeControl = (pdo >> 24) & 0x01;
  if (realTimeControl) {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.drawString("Real-time Control", 160, yPos + 10, 2);
  }
}

// 画面更新
void updateDisplay() {
  // メイン表示エリアをクリア
  M5.Lcd.fillRect(0, 40, 320, 180, BLACK);
  
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
  M5.Lcd.fillRect(0, 0, 320, 40, NAVY);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawCentreString("CH224A Source Capabilities", 160, 8, 2);
  
  M5.Lcd.setTextSize(1);
  String modeStr = (currentMode == DISPLAY_SRCAP) ? "Mode: SRCCAP" : "Mode: EPR_SRCCAP";
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.drawString(modeStr, 10, 28, 2);
}

// フッター描画
void drawFooter() {
  M5.Lcd.fillRect(0, 220, 320, 20, DARKGREY);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);
  
  M5.Lcd.drawString("READ", 15, 225, 2);
  M5.Lcd.drawString("MODE", 125, 225, 2);
  M5.Lcd.drawString("CLEAR", 225, 225, 2);
  
  // ボタン枠
  M5.Lcd.drawRect(10, 220, 40, 18, WHITE);
  M5.Lcd.drawRect(115, 220, 40, 18, WHITE);
  M5.Lcd.drawRect(215, 220, 50, 18, WHITE);
}
