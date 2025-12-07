#include <Wire.h>
#define CH224_ADDR 0x22
#define ADDR_START 0x60
#define ADDR_END   0x8F

uint8_t raw[ADDR_END - ADDR_START + 1];
uint8_t numPDO = 0;

void readREG(){
  Wire.beginTransmission(CH224_ADDR);
  Wire.write(ADDR_START);
  Wire.endTransmission(false);
  Wire.requestFrom(CH224_ADDR, sizeof(raw));
  if (Wire.available() != sizeof(raw)) {
    Serial.println("I2C error");
    delay(1000); return;
  }
  Serial.println("--- RAW DATA ---");
  for (int i = 0; i < sizeof(raw); i++){
    raw[i] = Wire.read();
    if(i % 16 == 15){
      Serial.printf("%02x\n", raw[i]);
    }else{
      Serial.printf("%02x ", raw[i]);
    }
  }
  Serial.println();
}

void decodeHEADER(){
  uint16_t header = raw[0] | (raw[1] << 8);
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
}

void decodePDO(uint8_t num){
  Serial.println("=== CH224A Source Capabilities ===");

  for (int i = 0; i < num; i++) {
    uint32_t pdo = raw[i*4 + 2] | (raw[i*4 + 3] << 8) | (raw[i*4 + 4] << 16) | (raw[i*4 + 5] << 24);
    // PDO番号を表示
    Serial.printf(" PDO%d:", i + 1);
    // PDOタイプに応じて解析
    uint8_t pdoType = (pdo >> 30) & 0x03;
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
      case 3: // Programmable Supply
        parsePPSPDO(pdo);
        break;
    }
  }
}

// 固定電源PDOを解析
void parseFixedPDO(uint32_t pdo) {
  Serial.print(" Fixed ");
  // 電圧 (50mV単位)
  uint16_t voltage_50mA = (pdo >> 10) & 0x3FF;
  float voltageV = voltage_50mA * 0.05f;
  // 電流 (10mA単位)
  uint16_t current_10mA = (pdo >> 0) & 0x3FF;
  float currentA = current_10mA * 0.01f;
  int peak = 100 + 25 * ((pdo >> 20) & 0x03);

/*
  String voltageStr = String(voltageV, 2) + "V";
  String currentStr = String(currentA, 2) + "A";
  
  Serial.print(voltageStr + " / ");
  Serial.println(currentStr);
*/

  Serial.printf("%5.2fV / %.2fA  (%dW)  Peak=%d%%\n", voltageV, currentA, (int)(voltageV * currentA + 0.5), peak);
}

// 可変電源PDOを解析
void parseVariablePDO(uint32_t pdo) {
  Serial.print(" Variable ");
  
  // 最小電圧 (100mV単位)
  uint16_t minVoltage = (pdo >> 8) & 0xFF;
  float minVoltageV = minVoltage * 0.1;
  
  // 最大電圧 (100mV単位)
  uint16_t maxVoltage = (pdo >> 17) & 0xFF;
  float maxVoltageV = maxVoltage * 0.1;
  
  // 最大電流 (50mA単位)
  uint16_t maxCurrent = (pdo >> 0) & 0x7F;
  float maxCurrentA = maxCurrent * 0.05;
  
  String voltageStr = String(minVoltageV, 2) + "V - " + String(maxVoltageV, 2) + "V";
  String currentStr = "Imax: " + String(maxCurrentA, 2) + "A";
  
  Serial.print(voltageStr + " / ");
  Serial.println(currentStr);
}

// PPS PDOを解析
void parsePPSPDO(uint32_t pdo) {
  Serial.print(" PPS ");
  
  // 最小電圧 (100mV単位)
  uint16_t minVoltage = (pdo >> 8) & 0xFF;
  float minVoltageV = minVoltage * 0.1;
  
  // 最大電圧 (100mV単位)
  uint16_t maxVoltage = (pdo >> 17) & 0xFF;
  float maxVoltageV = maxVoltage * 0.1;
  
  // 最大電流 (50mA単位)
  uint16_t maxCurrent = (pdo >> 0) & 0x7F;
  float maxCurrentA = maxCurrent * 0.05;
  
  String voltageStr = String(minVoltageV, 2) + "V - " + String(maxVoltageV, 2) + "V";
  String currentStr = "Max Current: " + String(maxCurrentA, 2) + "A";
  String powerStr = "Max Power: " + String(maxVoltageV * maxCurrentA, 2) + "W";
  
  Serial.print(voltageStr + " / ");
  Serial.print(currentStr + " , ");
  Serial.println(powerStr);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  delay(1000);
  Serial.println("CH224A PDO Decoder\n");
}

void loop() {
  readREG();
  decodeHEADER();
  decodePDO(numPDO);
  Serial.println();
  delay(3000);
}