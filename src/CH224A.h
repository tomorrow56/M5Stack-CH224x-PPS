/*
 * CH224A.h - CH224A USB PD Controller Library
 * 
 * CH224AチップをI2C経由で制御するためのライブラリ
 */

#ifndef CH224A_H
#define CH224A_H

#include <Arduino.h>
#include <Wire.h>

// CH224A I2Cアドレス
#define CH224A_I2C_ADDR_DEFAULT 0x22
#define CH224A_I2C_ADDR_ALT     0x23

// CH224Aレジスタアドレス
#define CH224A_REG_I2C_STATUS     0x09  // I2C状態寄存器
#define CH224A_REG_VOLTAGE_CTRL   0x0A  // 電圧制御寄存器
#define CH224A_REG_CURRENT_DATA   0x50  // 電流データ寄存器
#define CH224A_REG_AVS_VOLTAGE_L  0x51  // AVS電圧配置寄存器(低位)
#define CH224A_REG_AVS_VOLTAGE_H  0x52  // AVS電圧配置寄存器(高位)
#define CH224A_REG_PPS_VOLTAGE    0x53  // PPS電圧配置寄存器
#define CH224A_REG_PD_DATA_START  0x60  // PD電源データ寄存器開始アドレス
#define CH224A_REG_PD_DATA_END    0x8F  // PD電源データ寄存器終了アドレス

// 電圧モード定義
enum CH224A_VoltageMode {
  CH224A_VOLTAGE_5V   = 0,  // 5V固定
  CH224A_VOLTAGE_9V   = 1,  // 9V固定
  CH224A_VOLTAGE_12V  = 2,  // 12V固定
  CH224A_VOLTAGE_15V  = 3,  // 15V固定
  CH224A_VOLTAGE_20V  = 4,  // 20V固定
  CH224A_VOLTAGE_28V  = 5,  // 28V固定(EPR対応充電器のみ)
  CH224A_VOLTAGE_PPS  = 6,  // PPSモード
  CH224A_VOLTAGE_AVS  = 7   // AVSモード
};

class CH224A {
public:
  // コンストラクタ
  CH224A(uint8_t i2cAddress = CH224A_I2C_ADDR_DEFAULT);
  
  // 初期化
  bool begin(TwoWire &wirePort = Wire);
  
  // 固定電圧設定
  bool setFixedVoltage(CH224A_VoltageMode mode);
  bool setVoltage5V();
  bool setVoltage9V();
  bool setVoltage12V();
  bool setVoltage15V();
  bool setVoltage20V();
  bool setVoltage28V();
  
  // PPS電圧設定
  bool setPPSVoltage(float voltage);        // 電圧(V)で設定
  bool setPPSVoltageRaw(uint8_t voltage);   // 0.1V単位の生値で設定
  
  // AVS電圧設定
  bool setAVSVoltage(float voltage);        // 電圧(V)で設定
  bool setAVSVoltageRaw(uint16_t voltage);  // 0.1V単位の生値で設定
  
  // レジスタ読み書き
  bool writeRegister(uint8_t reg, uint8_t value);
  bool writeRegister16(uint8_t reg, uint16_t value);
  bool readRegister(uint8_t reg, uint8_t &value);
  bool readRegister16(uint8_t reg, uint16_t &value);
  
  // I2C通信確認
  bool isConnected();
  
  // I2Cアドレス取得
  uint8_t getI2CAddress() { return _i2cAddress; }

private:
  uint8_t _i2cAddress;
  TwoWire *_i2cPort;
};

#endif // CH224A_H
