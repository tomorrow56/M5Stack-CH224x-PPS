/*
 * CH224A.cpp - CH224A USB PD Controller Library Implementation
 */

#include "CH224A.h"

// コンストラクタ
CH224A::CH224A(uint8_t i2cAddress) {
  _i2cAddress = i2cAddress;
  _i2cPort = nullptr;
}

// 初期化
bool CH224A::begin(TwoWire &wirePort) {
  _i2cPort = &wirePort;
  
  // I2C通信確認
  return isConnected();
}

// I2C通信確認
bool CH224A::isConnected() {
  if (_i2cPort == nullptr) {
    return false;
  }
  
  _i2cPort->beginTransmission(_i2cAddress);
  return (_i2cPort->endTransmission() == 0);
}

// レジスタ書き込み(8ビット)
bool CH224A::writeRegister(uint8_t reg, uint8_t value) {
  if (_i2cPort == nullptr) {
    return false;
  }
  
  _i2cPort->beginTransmission(_i2cAddress);
  _i2cPort->write(reg);
  _i2cPort->write(value);
  uint8_t error = _i2cPort->endTransmission();
  
  delay(50);  // 設定反映待ち
  
  return (error == 0);
}

// レジスタ書き込み(16ビット)
bool CH224A::writeRegister16(uint8_t reg, uint16_t value) {
  if (_i2cPort == nullptr) {
    return false;
  }
  
  // 低位バイトを先に書き込み
  uint8_t lowByte = value & 0xFF;
  uint8_t highByte = (value >> 8) & 0xFF;
  
  bool success = true;
  success &= writeRegister(reg, lowByte);
  success &= writeRegister(reg + 1, highByte);
  
  return success;
}

// レジスタ読み込み(8ビット)
bool CH224A::readRegister(uint8_t reg, uint8_t &value) {
  if (_i2cPort == nullptr) {
    return false;
  }
  
  _i2cPort->beginTransmission(_i2cAddress);
  _i2cPort->write(reg);
  uint8_t error = _i2cPort->endTransmission(false);
  
  if (error != 0) {
    return false;
  }
  
  uint8_t bytesReceived = _i2cPort->requestFrom(_i2cAddress, (uint8_t)1);
  
  if (bytesReceived == 1) {
    value = _i2cPort->read();
    return true;
  }
  
  return false;
}

// レジスタ読み込み(16ビット)
bool CH224A::readRegister16(uint8_t reg, uint16_t &value) {
  if (_i2cPort == nullptr) {
    return false;
  }
  
  uint8_t lowByte, highByte;
  bool success = true;
  
  success &= readRegister(reg, lowByte);
  success &= readRegister(reg + 1, highByte);
  
  if (success) {
    value = ((uint16_t)highByte << 8) | lowByte;
  }
  
  return success;
}

// 固定電圧設定
bool CH224A::setFixedVoltage(CH224A_VoltageMode mode) {
  return writeRegister(CH224A_REG_VOLTAGE_CTRL, (uint8_t)mode);
}

// 5V設定
bool CH224A::setVoltage5V() {
  return setFixedVoltage(CH224A_VOLTAGE_5V);
}

// 9V設定
bool CH224A::setVoltage9V() {
  return setFixedVoltage(CH224A_VOLTAGE_9V);
}

// 12V設定
bool CH224A::setVoltage12V() {
  return setFixedVoltage(CH224A_VOLTAGE_12V);
}

// 15V設定
bool CH224A::setVoltage15V() {
  return setFixedVoltage(CH224A_VOLTAGE_15V);
}

// 20V設定
bool CH224A::setVoltage20V() {
  return setFixedVoltage(CH224A_VOLTAGE_20V);
}

// 28V設定(EPR対応充電器のみ)
bool CH224A::setVoltage28V() {
  return setFixedVoltage(CH224A_VOLTAGE_28V);
}

// PPS電圧設定(電圧値で指定)
bool CH224A::setPPSVoltage(float voltage) {
  // 電圧を0.1V単位に変換(例: 9.0V → 90)
  uint8_t voltageRaw = (uint8_t)(voltage * 10.0 + 0.5);
  return setPPSVoltageRaw(voltageRaw);
}

// PPS電圧設定(生値で指定)
bool CH224A::setPPSVoltageRaw(uint8_t voltage) {
  bool success = true;
  
  // PPS電圧値を設定
  success &= writeRegister(CH224A_REG_PPS_VOLTAGE, voltage);
  
  // PPSモードに切り替え
  success &= setFixedVoltage(CH224A_VOLTAGE_PPS);
  
  return success;
}

// AVS電圧設定(電圧値で指定)
bool CH224A::setAVSVoltage(float voltage) {
  // 電圧を0.1V単位に変換(例: 9.0V → 90)
  uint16_t voltageRaw = (uint16_t)(voltage * 10.0 + 0.5);
  return setAVSVoltageRaw(voltageRaw);
}

// AVS電圧設定(生値で指定)
bool CH224A::setAVSVoltageRaw(uint16_t voltage) {
  bool success = true;
  
  // AVS電圧値を設定(16ビット)
  success &= writeRegister16(CH224A_REG_AVS_VOLTAGE_L, voltage);
  
  // AVSモードに切り替え
  success &= setFixedVoltage(CH224A_VOLTAGE_AVS);
  
  return success;
}

// PDO解析機能
bool CH224A::readSourceCapabilities(uint8_t* data, size_t length) {
  if (length < (CH224A_REG_PD_DATA_END - CH224A_REG_PD_DATA_START + 1)) {
    return false;
  }
  
  bool success = true;
  for (size_t i = 0; i < length; i++) {
    uint8_t value;
    success &= readRegister(CH224A_REG_PD_DATA_START + i, value);
    if (success) {
      data[i] = value;
    }
  }
  
  return success;
}

uint8_t CH224A::getPDOCount(uint16_t header) {
  return (header >> 12) & 0x7;
}

void CH224A::decodeHeader(uint16_t header) {
  Serial.println("--- Message Header ---");
  Serial.printf("[15]:Extended: %d\n", (header >> 15) & 0x1);
  Serial.printf("[14:12]:Number of Data Objects: %d\n", getPDOCount(header));
  Serial.printf("[11:9]:MessageID: %d\n", (header >> 9) & 0x7);
  Serial.printf("[8]:Port Power Role: %d\n", (header >> 8) & 0x1);
  Serial.printf("[7:6]:Specification Revision: %d\n", (header >> 6) & 0x3);
  Serial.printf("[5]:Port Data Role: %d\n", (header >> 5) & 0x1);
  Serial.printf("[4:0]:Message Type: %d\n", (header >> 0) & 0x1F);
  Serial.println();
}

void CH224A::decodePDOs(uint8_t* data, uint8_t numPDO) {
  Serial.println("=== CH224A Source Capabilities ===");

  for (int i = 0; i < numPDO; i++) {
    uint32_t pdo = data[i*4 + 2] | (data[i*4 + 3] << 8) | (data[i*4 + 4] << 16) | (data[i*4 + 5] << 24);
    // PDO番号を表示
    Serial.printf(" PDO%d:", i + 1);
    // PDOタイプに応じて解析
    uint8_t pdoType = getPDOType(pdo);
    switch (pdoType) {
      case 0: // Fixed Supply
        {
          float voltage, current;
          parseFixedPDO(pdo, voltage, current);
          Serial.printf(" Fixed %5.2fV / %.2fA  (%dW)\n", voltage, current, (int)(voltage * current + 0.5));
        }
        break;
      case 1: // Variable Supply
        {
          float minVoltage, maxVoltage, current;
          parseVariablePDO(pdo, minVoltage, maxVoltage, current);
          Serial.printf(" Variable %.2fV - %.2fV / Imax: %.2fA\n", minVoltage, maxVoltage, current);
        }
        break;
      case 2: // Battery Supply
        {
          float voltage, current;
          parseFixedPDO(pdo, voltage, current);
          Serial.printf(" Battery %5.2fV / %.2fA  (%dW)\n", voltage, current, (int)(voltage * current + 0.5));
        }
        break;
      case 3: // Programmable Supply
        {
          float minVoltage, maxVoltage, current;
          parsePPSPDO(pdo, minVoltage, maxVoltage, current);
          Serial.printf(" PPS %.2fV - %.2fV / Max Current: %.2fA, Max Power: %.2fW\n", 
                      minVoltage, maxVoltage, current, maxVoltage * current);
        }
        break;
    }
  }
}

void CH224A::parseFixedPDO(uint32_t pdo, float& voltage, float& current) {
  // 電圧 (50mV単位)
  uint16_t voltage_50mA = (pdo >> 10) & 0x3FF;
  voltage = voltage_50mA * 0.05f;
  
  // 電流 (10mA単位)
  uint16_t current_10mA = (pdo >> 0) & 0x3FF;
  current = current_10mA * 0.01f;
}

void CH224A::parseVariablePDO(uint32_t pdo, float& minVoltage, float& maxVoltage, float& current) {
  // 最小電圧 (100mV単位)
  uint16_t minVoltageRaw = (pdo >> 8) & 0xFF;
  minVoltage = minVoltageRaw * 0.1f;
  
  // 最大電圧 (100mV単位)
  uint16_t maxVoltageRaw = (pdo >> 17) & 0xFF;
  maxVoltage = maxVoltageRaw * 0.1f;
  
  // 最大電流 (50mA単位)
  uint16_t maxCurrent = (pdo >> 0) & 0x7F;
  current = maxCurrent * 0.05f;
}

void CH224A::parsePPSPDO(uint32_t pdo, float& minVoltage, float& maxVoltage, float& current) {
  // 最小電圧 (100mV単位)
  uint16_t minVoltageRaw = (pdo >> 8) & 0xFF;
  minVoltage = minVoltageRaw * 0.1f;
  
  // 最大電圧 (100mV単位)
  uint16_t maxVoltageRaw = (pdo >> 17) & 0xFF;
  maxVoltage = maxVoltageRaw * 0.1f;
  
  // 最大電流 (50mA単位)
  uint16_t maxCurrent = (pdo >> 0) & 0x7F;
  current = maxCurrent * 0.05f;
}

uint8_t CH224A::getPDOType(uint32_t pdo) {
  return (pdo >> 30) & 0x03;
}
