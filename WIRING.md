# 配線ガイド

このドキュメントでは、M5Stack Core BasicとCH224Aチップの詳細な配線方法を説明します。

## 必要な部品

### 主要部品
- M5Stack Core Basic × 1
- CH224A USB PDチップ(ESSOP10またはDFN10パッケージ) × 1
- USB Type-Cコネクタ × 1
- 1μFセラミックコンデンサ × 1

### オプション部品
- LED × 1(Power Good表示用)
- 抵抗(330Ω～1kΩ) × 1(LED用)
- プルアップ抵抗(4.7kΩ) × 2(I2C用、必要な場合)

## CH224Aピン配置

### ESSOP10/DFN10パッケージ

```
        CH224A
    +------------+
  0 | GND        | 10  PG
  1 | VHV        | 9   CFG1
  2 | CFG2/SCL   | 8   VBUS
  3 | CFG3/SDA   | 7   CC1
  4 | DP         | 6   CC2
  5 | DM         |
    +------------+
```

## 基本配線図

### M5Stack ⇔ CH224A接続

```
M5Stack Core Basic          CH224A
+------------------+        +------------------+
|                  |        |                  |
| GPIO21 (SDA) ----+--------+--- Pin 3 (SDA)  |
| GPIO22 (SCL) ----+--------+--- Pin 2 (SCL)  |
| GND          ----+--------+--- Pin 0 (GND)  |
|                  |        |                  |
+------------------+        +------------------+
```

### CH224A完全配線図

```
USB Type-C Connector                CH224A                    M5Stack
+-------------------+          +----------------+         +-----------+
|                   |          |                |         |           |
| VBUS -------------|----------| Pin 1 (VHV)    |         |           |
|                   |    |     |                |         |           |
|                   |  [1μF]   | Pin 8 (VBUS)---|         |           |
|                   |    |     |                |         |           |
| GND --------------|----+-----| Pin 0 (GND) ---|---------|--- GND    |
|                   |          |                |         |           |
| D+ ---------------|----------| Pin 4 (DP)     |         |           |
| D- ---------------|----------| Pin 5 (DM)     |         |           |
|                   |          |                |         |           |
| CC1 --------------|----------| Pin 7 (CC1)    |         |           |
| CC2 --------------|----------| Pin 6 (CC2)    |         |           |
|                   |          |                |         |           |
+-------------------+          | Pin 2 (SCL) ---|---------|--- GPIO22 |
                               | Pin 3 (SDA) ---|---------|--- GPIO21 |
                               |                |         |           |
                               | Pin 9 (CFG1)   |         |           |
                               | (フローティング)|         |           |
                               |                |         |           |
                               | Pin 10 (PG) ---+--- [LED+抵抗] --- GND
                               |                |         |           |
                               +----------------+         +-----------+
```

## 詳細配線手順

### 1. 電源配線

CH224AのVHVピン(Pin 1)にUSB Type-CのVBUSを接続します。VHVとGND間に1μFのセラミックコンデンサを配置してください。

```
VBUS ---+--- VHV (Pin 1)
        |
      [1μF]
        |
       GND (Pin 0)
```

### 2. USB Data配線

USB Type-CのD+/D-をCH224AのDP/DMピンに接続します。

```
USB D+ --- DP (Pin 4)
USB D- --- DM (Pin 5)
```

### 3. Type-C CC配線

Type-CのCC1/CC2ピンをCH224Aに接続します。これらのピンはUSB PDの通信に使用されます。

```
Type-C CC1 --- CC1 (Pin 7)
Type-C CC2 --- CC2 (Pin 6)
```

### 4. 電圧検出配線

VBUSをCH224AのVBUSピン(Pin 8)にも接続します。これは電圧検出用です。

```
VBUS --- VBUS (Pin 8)
```

### 5. I2C配線(M5Stack接続)

M5StackのI2CピンをCH224Aに接続します。

```
M5Stack GPIO21 (SDA) --- CFG3/SDA (Pin 3)
M5Stack GPIO22 (SCL) --- CFG2/SCL (Pin 2)
M5Stack GND          --- GND (Pin 0)
```

**重要**: CFG2とCFG3ピンをI2Cとして使用する場合、これらのピンに抵抗を接続しないでください。フローティング状態にします。

### 6. CFG1ピン設定

CFG1ピン(Pin 9)はフローティング(未接続)のままにします。I2C制御を使用する場合、このピンは使用しません。

### 7. Power Goodピン(オプション)

PGピン(Pin 10)は、電源が正常に供給されているときにLOWになります。LEDで状態を表示する場合:

```
PG (Pin 10) --- [330Ω抵抗] --- LED(アノード)
                               LED(カソード) --- GND
```

### 8. I2Cプルアップ抵抗(必要な場合)

M5StackのI2Cピンには内部プルアップがありますが、配線が長い場合や動作が不安定な場合は、外部プルアップ抵抗(4.7kΩ)を追加してください。

```
3.3V --- [4.7kΩ] --- SDA (GPIO21)
3.3V --- [4.7kΩ] --- SCL (GPIO22)
```

## 配線チェックリスト

配線完了後、以下の項目を確認してください:

- [ ] VHVとGND間に1μFコンデンサが接続されている
- [ ] USB D+がDPピンに接続されている
- [ ] USB D-がDMピンに接続されている
- [ ] Type-C CC1がCC1ピンに接続されている
- [ ] Type-C CC2がCC2ピンに接続されている
- [ ] VBUSがVBUSピン(Pin 8)に接続されている
- [ ] M5Stack GPIO21がCFG3/SDA(Pin 3)に接続されている
- [ ] M5Stack GPIO22がCFG2/SCL(Pin 2)に接続されている
- [ ] M5StackとCH224AのGNDが共通接続されている
- [ ] CFG1ピン(Pin 9)がフローティング状態である
- [ ] CFG2/CFG3ピンに抵抗が接続されていない

## I2Cアドレスの確認

CH224AのI2Cアドレスは通常0x22ですが、0x23の場合もあります。スケッチ内で正しいアドレスを設定してください。

I2Cスキャナーを使用してアドレスを確認する場合:

```cpp
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  
  Serial.println("I2C Scanner");
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
}

void loop() {}
```

## トラブルシューティング

### I2C通信エラー

**症状**: CH224Aが応答しない

**確認事項**:
1. SDA/SCLの配線が正しいか
2. GNDが共通接続されているか
3. CFG2/CFG3ピンに抵抗が接続されていないか
4. I2Cアドレスが正しいか(0x22または0x23)

**対策**:
- プルアップ抵抗(4.7kΩ)をSDA/SCLに追加
- 配線を短くする
- I2Cスキャナーでアドレスを確認

### 電圧が変わらない

**症状**: 電圧設定を変更しても出力電圧が変わらない

**確認事項**:
1. USB PD充電器がPPSをサポートしているか
2. USB Type-Cケーブルがフル機能対応か
3. CC1/CC2ピンが正しく接続されているか
4. VBUSピン(Pin 8)が接続されているか

**対策**:
- PPS対応のUSB PD充電器を使用
- USB PD対応のケーブルを使用
- CC1/CC2の配線を確認

### Power Goodが動作しない

**症状**: PGピンの出力が変化しない

**確認事項**:
1. VHVに電源が供給されているか
2. USB PD通信が正常に行われているか

**対策**:
- VHVとGND間の電圧を測定(5V以上あるか確認)
- USB PD充電器を交換してみる

## 安全上の注意

1. **短絡防止**: 配線時は必ず電源を切ってください
2. **極性確認**: コンデンサやLEDの極性を確認してください
3. **電圧範囲**: CH224Aの最大入力電圧(30V)を超えないでください
4. **発熱**: 高電圧・高電流使用時は発熱に注意してください

## 参考資料

- CH224Aデータシート: https://wch.cn
- USB Type-C仕様: https://www.usb.org
- M5Stack公式ドキュメント: https://docs.m5stack.com
