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
 * - ボタンA: データを更新
 * - ボタンB: 表示モード切替(SRCCAP/EPR_SRCCAP/HEADER)
 * - ボタンC: メッセージヘッダー表示
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
  DISPLAY_EPR_SRCAP = 1,
  DISPLAY_HEADER = 2
};

// グローバル変数
DisplayMode currentMode = DISPLAY_SRCAP;
uint8_t pdData[48];  // 0x60-0x8Fのデータを格納（48バイト）
uint8_t numPDO = 0;  // PDO数（ヘッダーから取得）
bool dataValid = false;
bool eprModeCapable = false;  // EPR Mode Capableフラグ

// 関数プロトタイプ
bool readPDData();
void decodeHeader();
void checkEPRCapable();
void displaySRCCAP();
void displayEPRSRCCAP();
void displayMessageHeader();
void displayPDODetails(int pdoCount, int dataOffset, bool isEPR);
void parseFixedPDO(uint32_t pdo);
void parseVariablePDO(uint32_t pdo);
void parsePPSPDO(uint32_t pdo);
void parseEPR_AVS_PDO(uint32_t pdo);
void parseEPR_Fixed_PDO(uint32_t pdo);
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
  Serial.begin(115200);
  
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
  
  // ボタンB: 表示モード切替 (SRCCAP -> EPR_SRCCAP -> HEADER -> SRCCAP)
  if (M5.BtnB.wasPressed()) {
    if (currentMode == DISPLAY_SRCAP) {
      currentMode = DISPLAY_EPR_SRCAP;
    } else if (currentMode == DISPLAY_EPR_SRCAP) {
      currentMode = DISPLAY_HEADER;
    } else {
      currentMode = DISPLAY_SRCAP;
    }
    updateDisplay();
  }
  
  // ボタンC: メッセージヘッダー表示
  if (M5.BtnC.wasPressed()) {
    currentMode = DISPLAY_HEADER;
    updateDisplay();
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
    Serial.println("--- RAW DATA ---");
    for (int i = 0; i < 48; i++) {
      pdData[i] = Wire.read();
      if (i % 16 == 15) {
        Serial.printf("%02x\n", pdData[i]);
      } else {
        Serial.printf("%02x ", pdData[i]);
      }
    }
    Serial.println();
    
    // ヘッダーをデコードしてPDO数を取得
    decodeHeader();
    return true;
  }
  
  return false;
}

// Message Headerをデコード
void decodeHeader() {
  uint16_t header = pdData[0] | (pdData[1] << 8);
  numPDO = (header >> 12) & 0x7;
  
  Serial.println("--- Message Header ---");
  Serial.printf("[15]:Extended: %d\n", (header >> 15) & 0x1);
  Serial.printf("[14:12]:Number of Data Objects: %d\n", numPDO);
  Serial.printf("[11:9]:MessageID: %d\n", (header >> 9) & 0x7);
  Serial.printf("[8]:Port Power Role: %d\n", (header >> 8) & 0x1);
  Serial.printf("[7:6]:Specification Revision: %d\n", (header >> 6) & 0x3);
  Serial.printf("[5]:Port Data Role: %d\n", (header >> 5) & 0x1);
  Serial.printf("[4:0]:Message Type: %d\n", (header >> 0) & 0x1F);
  Serial.println();
  
  // EPR Capable判定
  checkEPRCapable();
}

// Fixed PDO1のbit[23]でEPR Mode Capableを判定
void checkEPRCapable() {
  // PDO1はオフセット2から（ヘッダー2バイトの後）
  uint32_t pdo1 = pdData[2] | 
                  (pdData[3] << 8) | 
                  (pdData[4] << 16) | 
                  (pdData[5] << 24);
  
  // PDOタイプを確認（bit[31:30] = 00 でFixed Supply）
  uint8_t pdoType = (pdo1 >> 30) & 0x03;
  
  if (pdoType == 0) {
    // Fixed PDOのbit[23]がEPR Mode Capable
    eprModeCapable = (pdo1 >> 23) & 0x01;
    Serial.printf("EPR Mode Capable: %s\n", eprModeCapable ? "Yes" : "No");
  } else {
    eprModeCapable = false;
    Serial.println("PDO1 is not Fixed Supply, EPR check skipped");
  }
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
  
  // PDO詳細表示（ヘッダーから取得したPDO数を使用、オフセット2から、通常モード）
  displayPDODetails(numPDO, 2, false);
}

// EPR_SRCCAPデータを表示
void displayEPRSRCCAP() {
  M5.Display.setCursor(0, 40);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(CYAN);
  M5.Display.println("EPR_SRCCAP Analysis");
  
  if (!dataValid) {
    M5.Display.setTextColor(RED);
    M5.Display.println("No data available");
    return;
  }
  
  // EPR Mode Capable判定を表示
  M5.Display.setTextColor(WHITE);
  M5.Display.print("EPR Mode Capable: ");
  if (eprModeCapable) {
    M5.Display.setTextColor(GREEN);
    M5.Display.println("Yes");
  } else {
    M5.Display.setTextColor(RED);
    M5.Display.println("No");
    M5.Display.setTextColor(YELLOW);
    M5.Display.println("\nSource does not support EPR");
    return;
  }
  
  // EPR PDOは通常のSRCCAP PDOの後ろに追加されている
  // オフセット = 2(ヘッダー) + numPDO * 4(通常PDO)
  int eprDataOffset = 2 + numPDO * 4;
  
  Serial.println("--- EPR_SRCCAP Data ---");
  Serial.printf("SRCCAP PDO Count: %d\n", numPDO);
  Serial.printf("EPR Data Offset: %d (0x%02X)\n", eprDataOffset, 0x60 + eprDataOffset);
  
  // デバッグ: 全RAWデータをPDO単位で表示（リトルエンディアンとビッグエンディアン両方）
  Serial.println("All PDOs:");
  for (int i = 0; i < 12; i++) {
    int idx = 2 + i * 4;
    if (idx + 3 >= 48) break;
    // リトルエンディアン
    uint32_t pdo_le = pdData[idx] | (pdData[idx+1] << 8) | (pdData[idx+2] << 16) | (pdData[idx+3] << 24);
    // ビッグエンディアン
    uint32_t pdo_be = (pdData[idx] << 24) | (pdData[idx+1] << 16) | (pdData[idx+2] << 8) | pdData[idx+3];
    uint16_t voltage_le = (pdo_le >> 10) & 0x3FF;
    uint16_t voltage_be = (pdo_be >> 10) & 0x3FF;
    Serial.printf("  [%d] offset=%d: LE=0x%08X (%.1fV) BE=0x%08X (%.1fV)\n", 
                  i+1, idx, pdo_le, voltage_le * 0.05f, pdo_be, voltage_be * 0.05f);
    if (pdo_le == 0) break;
  }
  
  // EPR PDOはSRCCAPの直後から（ヘッダーなし）
  int eprPdoOffset = eprDataOffset;
  Serial.printf("EPR PDO Offset: %d\n", eprPdoOffset);
  
  // 最初のEPR PDOを確認
  if (eprPdoOffset + 3 >= 48) {
    M5.Display.setTextColor(YELLOW);
    M5.Display.println("\nNo space for EPR PDOs");
    return;
  }
  
  // リトルエンディアンで読む
  uint32_t firstPdo = pdData[eprPdoOffset] | 
                      (pdData[eprPdoOffset + 1] << 8) | 
                      (pdData[eprPdoOffset + 2] << 16) | 
                      (pdData[eprPdoOffset + 3] << 24);
  
  Serial.printf("EPR First PDO: 0x%08X\n", firstPdo);
  
  if (firstPdo == 0 || firstPdo == 0xFFFFFFFF) {
    M5.Display.setTextColor(YELLOW);
    M5.Display.println("\nNo EPR PDOs available");
    
    // RAWデータを表示（デバッグ用）
    M5.Display.setTextColor(WHITE);
    M5.Display.printf("\nEPR RAW: %02X %02X %02X %02X\n", 
                      pdData[eprDataOffset], pdData[eprDataOffset+1], 
                      pdData[eprDataOffset+2], pdData[eprDataOffset+3]);
    return;
  }
  
  // EPR PDO数をカウント（残りのバッファ領域内で）
  int eprPdoCount = 0;
  int maxEprPdos = (48 - eprPdoOffset) / 4;  // 残りのスペースで格納可能なPDO数
  
  for (int i = 0; i < maxEprPdos; i++) {
    int idx = eprPdoOffset + i * 4;
    if (idx + 3 >= 48) break;
    
    // リトルエンディアン
    uint32_t pdo = pdData[idx] | 
                   (pdData[idx + 1] << 8) | 
                   (pdData[idx + 2] << 16) | 
                   (pdData[idx + 3] << 24);
    if (pdo != 0 && pdo != 0xFFFFFFFF) {
      eprPdoCount++;
    } else {
      break;
    }
  }
  
  Serial.printf("EPR PDO Count: %d\n", eprPdoCount);
  
  // EPR PDO詳細表示（ヘッダー後のオフセットから、EPRモード）
  displayPDODetails(eprPdoCount, eprPdoOffset, true);
}

// PDO詳細情報を表示
// dataOffset: SRCCAPは2、EPR_SRCCAPはnumPDO*4+2
// isEPR: EPR PDOの場合はtrue
void displayPDODetails(int pdoCount, int dataOffset, bool isEPR) {
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);

  // PDO数表示
  M5.Display.setTextColor(GREEN);
  String countStr = "PDO Count: " + String(pdoCount);
  M5.Display.println(countStr);
  
  Serial.println("=== CH224A Source Capabilities ===");
  
  // 各PDOを解析して表示
  for (int i = 0; i < pdoCount; i++) {
    // PDOは4バイト（32ビット）
    int idx = dataOffset + i * 4;
    if (idx + 3 >= 48) break;  // バッファオーバーフロー防止
    
    // リトルエンディアン
    uint32_t pdo = pdData[idx] | 
                   (pdData[idx + 1] << 8) | 
                   (pdData[idx + 2] << 16) | 
                   (pdData[idx + 3] << 24);

    M5.Display.setTextColor(YELLOW);
    M5.Display.print("PDO" + String(i + 1) + ": ");
    
    Serial.printf(" PDO%d:", i + 1);

    // PDOタイプに応じて解析
    uint8_t pdoType = (pdo >> 30) & 0x03;
    
    if (isEPR) {
      // EPR PDOの場合、bit[31:30]で判定
      // EPR Fixed: bit[31:30] = 00
      // EPR AVS:   bit[31:30] = 11, bit[29:28] = 01
      uint8_t eprType = (pdo >> 30) & 0x03;
      
      Serial.printf(" [RAW:0x%08X, type=%d] ", pdo, eprType);
      
      if (eprType == 0) {
        // EPR Fixed Supply (bit[31:30] = 00)
        parseEPR_Fixed_PDO(pdo);
      } else if (eprType == 3) {
        // EPR AVS PDO (bit[31:30] = 11)
        parseEPR_AVS_PDO(pdo);
      } else {
        // 不明なタイプ
        M5.Display.setTextColor(RED);
        M5.Display.printf("Unknown EPR type=%d\n", eprType);
        Serial.printf(" Unknown EPR PDO type: %d\n", eprType);
      }
    } else {
      // 通常のSRCCAP PDO
      switch (pdoType) {
        case 0: // Fixed Supply
          parseFixedPDO(pdo);
          break;
        case 1: // Variable Supply
          parseVariablePDO(pdo);
          break;
        case 2: // Battery Supply
          parseFixedPDO(pdo);
          break;
        case 3: // Programmable Power Supply (PPS)
          parsePPSPDO(pdo);
          break;
      }
    }
  }
}

// 固定電源PDOを解析
void parseFixedPDO(uint32_t pdo) {
  // 電圧 (50mV単位)
  uint16_t voltage_50mV = (pdo >> 10) & 0x3FF;
  float voltageV = voltage_50mV * 0.05f;
  
  // 電流 (10mA単位)
  uint16_t current_10mA = (pdo >> 0) & 0x3FF;
  float currentA = current_10mA * 0.01f;
  
  // ピーク電流
  int peak = 100 + 25 * ((pdo >> 20) & 0x03);
  
  // 電力計算
  int powerW = (int)(voltageV * currentA + 0.5);
  
  // ディスプレイ表示
  M5.Display.setTextColor(CYAN);
  M5.Display.print("Fixed ");
  M5.Display.setTextColor(WHITE);
  M5.Display.printf("%.1fV/%.2fA(%dW)\n", voltageV, currentA, powerW);
  
  // シリアル出力
  Serial.printf(" Fixed %5.2fV / %.2fA  (%dW)  Peak=%d%%\n", voltageV, currentA, powerW, peak);
}

// 可変電源PDOを解析
void parseVariablePDO(uint32_t pdo) {
  // 最小電圧 (100mV単位)
  uint16_t minVoltage = (pdo >> 8) & 0xFF;
  float minVoltageV = minVoltage * 0.1f;
  
  // 最大電圧 (100mV単位)
  uint16_t maxVoltage = (pdo >> 17) & 0xFF;
  float maxVoltageV = maxVoltage * 0.1f;
  
  // 最大電流 (50mA単位)
  uint16_t maxCurrent = (pdo >> 0) & 0x7F;
  float maxCurrentA = maxCurrent * 0.05f;
  
  // ディスプレイ表示
  M5.Display.setTextColor(CYAN);
  M5.Display.print("Variable ");
  M5.Display.setTextColor(WHITE);
  M5.Display.printf("%.1f-%.1fV/%.2fA\n", minVoltageV, maxVoltageV, maxCurrentA);
  
  // シリアル出力
  Serial.printf(" Variable %.2fV - %.2fV / Imax: %.2fA\n", minVoltageV, maxVoltageV, maxCurrentA);
}

// PPS PDOを解析
void parsePPSPDO(uint32_t pdo) {
  // 最小電圧 (100mV単位)
  uint16_t minVoltage = (pdo >> 8) & 0xFF;
  float minVoltageV = minVoltage * 0.1f;
  
  // 最大電圧 (100mV単位)
  uint16_t maxVoltage = (pdo >> 17) & 0xFF;
  float maxVoltageV = maxVoltage * 0.1f;
  
  // 最大電流 (50mA単位)
  uint16_t maxCurrent = (pdo >> 0) & 0x7F;
  float maxCurrentA = maxCurrent * 0.05f;
  
  // 最大電力
  float maxPowerW = maxVoltageV * maxCurrentA;
  
  // ディスプレイ表示
  M5.Display.setTextColor(MAGENTA);
  M5.Display.print("PPS ");
  M5.Display.setTextColor(WHITE);
  M5.Display.printf("%.1f-%.1fV/%.2fA(%.1fW)\n", minVoltageV, maxVoltageV, maxCurrentA, maxPowerW);
  
  // シリアル出力
  Serial.printf(" PPS %.2fV - %.2fV / Max Current: %.2fA , Max Power: %.2fW\n", 
                minVoltageV, maxVoltageV, maxCurrentA, maxPowerW);
}

// EPR Fixed PDOを解析
void parseEPR_Fixed_PDO(uint32_t pdo) {
  // EPR Fixed PDOのフォーマット (USB PD 3.1):
  // 通常のFixed PDOと同じフォーマット
  // bit[31:30]: 00 (Fixed Supply)
  // bit[29:28]: Peak Current
  // bit[19:10]: Voltage (50mV単位, 10ビット)
  // bit[9:0]:   Max Current (10mA単位, 10ビット)
  
  uint8_t peakCurrent = (pdo >> 28) & 0x03;
  uint16_t voltage = (pdo >> 10) & 0x3FF;   // 10ビット
  uint16_t maxCurrent = pdo & 0x3FF;         // 10ビット
  
  float voltageV = voltage * 0.05f;          // 50mV単位
  float currentA = maxCurrent * 0.01f;       // 10mA単位
  int peak = 100 + 25 * peakCurrent;
  int powerW = (int)(voltageV * currentA + 0.5f);
  
  // ディスプレイ表示
  M5.Display.setTextColor(CYAN);
  M5.Display.print("EPR Fixed ");
  M5.Display.setTextColor(WHITE);
  M5.Display.printf("%.1fV/%.2fA(%dW)\n", voltageV, currentA, powerW);
  
  // シリアル出力
  Serial.printf(" EPR Fixed %.1fV / %.2fA (%dW) Peak=%d%%\n", 
                voltageV, currentA, powerW, peak);
}

// EPR AVS PDOを解析
void parseEPR_AVS_PDO(uint32_t pdo) {
  // EPR AVS PDOのフォーマット (USB PD 3.1):
  // bit[31:28]: PDO Type (0001 = EPR AVS)
  // bit[27:26]: Peak Current
  // bit[25:17]: Max Voltage (100mV単位, 9ビット)
  // bit[16]:    Reserved
  // bit[15:8]:  Min Voltage (100mV単位, 8ビット)
  // bit[7:0]:   PDP (1W単位)
  
  uint8_t peakCurrent = (pdo >> 26) & 0x03;
  uint16_t maxVoltage = (pdo >> 17) & 0x1FF;  // 9ビット
  uint16_t minVoltage = (pdo >> 8) & 0xFF;    // 8ビット
  uint8_t pdp = pdo & 0xFF;                    // PDP (Power)
  
  float maxVoltageV = maxVoltage * 0.1f;  // 100mV単位
  float minVoltageV = minVoltage * 0.1f;  // 100mV単位
  int peak = 100 + 25 * peakCurrent;
  
  // ディスプレイ表示
  M5.Display.setTextColor(GREEN);
  M5.Display.print("EPR AVS ");
  M5.Display.setTextColor(WHITE);
  M5.Display.printf("%.1f-%.1fV %dW\n", minVoltageV, maxVoltageV, pdp);
  
  // シリアル出力
  Serial.printf(" EPR AVS %.1fV - %.1fV / PDP: %dW / Peak=%d%%\n", 
                minVoltageV, maxVoltageV, pdp, peak);
}

// メッセージヘッダーをディスプレイに表示
void displayMessageHeader() {
  M5.Display.setCursor(0, 40);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(CYAN);
  M5.Display.println("Message Header Analysis");
  M5.Display.println();
  
  if (!dataValid) {
    M5.Display.setTextColor(RED);
    M5.Display.println("No data available");
    return;
  }
  
  uint16_t header = pdData[0] | (pdData[1] << 8);
  
  // RAWデータ表示
  M5.Display.setTextColor(YELLOW);
  M5.Display.printf("RAW: 0x%04X\n\n", header);
  
  // 各フィールドを表示
  M5.Display.setTextColor(WHITE);
  
  M5.Display.print("[15] Extended: ");
  M5.Display.setTextColor(GREEN);
  M5.Display.println((header >> 15) & 0x1);
  
  M5.Display.setTextColor(WHITE);
  M5.Display.print("[14:12] Num of Data Objects: ");
  M5.Display.setTextColor(GREEN);
  M5.Display.println(numPDO);
  
  M5.Display.setTextColor(WHITE);
  M5.Display.print("[11:9] MessageID: ");
  M5.Display.setTextColor(GREEN);
  M5.Display.println((header >> 9) & 0x7);
  
  M5.Display.setTextColor(WHITE);
  M5.Display.print("[8] Port Power Role: ");
  M5.Display.setTextColor(GREEN);
  M5.Display.println((header >> 8) & 0x1);
  
  M5.Display.setTextColor(WHITE);
  M5.Display.print("[7:6] Spec Revision: ");
  M5.Display.setTextColor(GREEN);
  uint8_t specRev = (header >> 6) & 0x3;
  M5.Display.printf("%d (PD%s)\n", specRev, specRev == 0 ? "1.0" : specRev == 1 ? "2.0" : specRev == 2 ? "3.0" : "3.1");
  
  M5.Display.setTextColor(WHITE);
  M5.Display.print("[5] Port Data Role: ");
  M5.Display.setTextColor(GREEN);
  M5.Display.println((header >> 5) & 0x1);
  
  M5.Display.setTextColor(WHITE);
  M5.Display.print("[4:0] Message Type: ");
  M5.Display.setTextColor(GREEN);
  uint8_t msgType = (header >> 0) & 0x1F;
  M5.Display.printf("%d", msgType);
  if (msgType == 1) M5.Display.print(" (Source_Cap)");
  M5.Display.println();
}

// 画面更新
void updateDisplay() {
  // メイン表示エリアをクリア
  M5.Display.fillRect(0, 40, 320, 180, BLACK);
  
  // ヘッダーを再描画（モード表示更新のため）
  drawHeader();
  
  // 現在のモードに応じてデータを表示
  if (currentMode == DISPLAY_SRCAP) {
    displaySRCCAP();
  } else if (currentMode == DISPLAY_EPR_SRCAP) {
    displayEPRSRCCAP();
  } else {
    displayMessageHeader();
  }
  
  drawFooter();
}

// ヘッダー描画
void drawHeader() {
  M5.Display.fillRect(0, 0, 320, 40, NAVY);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);
  M5.Display.drawCentreString("CH224A Source Capabilities", 160, 3, 2);
  
  M5.Display.setTextSize(1);
  String modeStr;
  if (currentMode == DISPLAY_SRCAP) {
    modeStr = "Mode: SRCCAP";
  } else if (currentMode == DISPLAY_EPR_SRCAP) {
    modeStr = "Mode: EPR_SRCCAP";
  } else {
    modeStr = "Mode: HEADER";
  }
  M5.Display.setTextColor(YELLOW);
  M5.Display.drawString(modeStr, 10, 25, 2);
}

// フッター描画
void drawFooter() {
  M5.Display.fillRect(0, 220, 320, 20, BLUE);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);
  
  M5.Display.drawString("READ", 25, 222, 2);
  M5.Display.drawString("MODE", 135, 222, 2);
  M5.Display.drawString("HDR", 240, 222, 2);

}
