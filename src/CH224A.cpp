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
