# M5Stack-CH224x-PPS

M5Stack（M5Unified対応）でCH224A/CH224Q USB PDコントローラをI2C経由で制御し、USB PD PPS/AVSモードで電圧を調整するArduinoライブラリおよびサンプルスケッチ集です。

## 機能

- **固定電圧モード**: 5V/9V/12V/15V/20V/28Vの固定電圧から選択
- **PPSモード**: 5.0V～21.0Vの範囲で0.1Vステップで電圧を調整
- **AVSモード**: 可変電圧設定（16ビット精度）
- **Source Capabilities解析**: SRCCAP/EPR_SRCCAPメッセージの詳細表示
- **PDOタイプ識別**: Fixed/Battery/Variable/PPS Supplyの自動判別
- **CH224Aライブラリ**: 再利用可能なライブラリクラス（`src/CH224A.h`）

## プロジェクト構成

```
M5Stack-CH224x-PPS/
├── src/
│   ├── CH224A.h          # CH224Aライブラリヘッダ
│   └── CH224A.cpp        # CH224Aライブラリ実装
├── examples/
│   ├── M5Stack_CH224A_PPS/               # PPS電圧制御（GUI付き）
│   ├── M5Stack_CH224A_Source_Capabilities/ # Source Capabilities解析
│   ├── Simple_Test/                      # 固定電圧自動切替テスト
│   ├── PPS_Test/                         # PPS電圧スイープテスト
│   ├── I2C_Scanner/                      # I2Cデバイススキャナ
│   └── CH224A_PDO_Test/                  # PDOデコードテスト（シリアル出力）
├── WIRING.md             # 詳細配線ガイド
├── LICENSE               # MITライセンス
└── README.md
```

## ハードウェア要件

### 必要な部品

- **M5Stack**（M5Unified対応デバイス: Core Basic, Core2, CoreS3等）
- **CH224A/CH224K** USB PDコントローラ（ESSOP10/DFN10パッケージ）
- **USB Type-Cコネクタ**
- **1μFセラミックコンデンサ**（VHV-GND間）

### ピン接続

| M5Stack       | CH224A          | 説明             |
|---------------|-----------------|------------------|
| GPIO21        | Pin 3 (CFG3/SDA)| I2C データ線     |
| GPIO22        | Pin 2 (CFG2/SCL)| I2C クロック線   |
| GND           | Pin 0 (GND)     | グランド         |
| GPIO2 (※)    | VBUSEN          | 出力イネーブル   |
| GPIO12 (※)   | Pin 10 (PG)     | Power Good入力   |

※ サンプルスケッチで使用。用途に応じて変更可能。

**重要**: CFG2/CFG3ピンをI2Cとして使用する場合、これらのピンに抵抗を接続しないでください（フローティング状態にする）。

詳細な配線については [WIRING.md](WIRING.md) を参照してください。

## ソフトウェア要件

### Arduino IDE設定

1. **M5Stackボードマネージャの追加**:
   - Arduino IDE → ファイル → 環境設定
   - 「追加のボードマネージャのURL」に以下を追加:
     ```
     https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
     ```

2. **M5Stackボードのインストール**:
   - ツール → ボード → ボードマネージャ
   - "M5Stack"で検索してインストール

3. **M5Unifiedライブラリのインストール**:
   - スケッチ → ライブラリをインクルード → ライブラリを管理
   - "M5Unified"で検索してインストール

### ボード設定

- **ボード**: M5Stack-Core-ESP32（または使用するM5Stackデバイス）
- **Upload Speed**: 921600
- **Flash Frequency**: 80MHz

## サンプルスケッチ

### M5Stack_PDtrigger_PPS

M5Stack PD Analyzer - USB PD充電器の解析と電圧制御を行うメインアプリケーション。

**主な機能**:
- 固定電圧モード（5V/9V/12V/15V/20V）の切り替え
- PPSモードでの電圧調整（0.1Vステップ）
- PDO Decode画面でSource Capabilitiesの詳細表示
- リアルタイム電圧・電流測定とPG（Power Good）表示

**操作方法**:
- **ボタンA（左）**: 電圧を下げる / PDO=0時にPDO Decode画面へ
- **ボタンB（中央）**: 出力ON/OFF切り替え
- **ボタンC（右）**: 電圧を上げる / PDO=4時にPPS制御へ / PDO Decode画面からEXIT

### M5Stack_CH224A_PPS

旧バージョンのPPS電圧制御GUIアプリケーション。

### test/フォルダ

以下のテスト・デバッグ用スケッチを含みます。

#### CH224A_PDO_Test
PDOデータをシリアルモニタに出力するデバッグ用スケッチ。Source Capabilitiesの生データと解析結果を表示します。

#### I2C_Scanner
I2Cバス上のデバイスをスキャンしてアドレスを表示。CH224Aの接続確認に使用します。

#### PPS_Test
PPS電圧を5.5V～20.0Vの範囲で0.5Vステップで自動スイープするテストスケッチ。

#### Simple_Test
2秒ごとに固定電圧（5V→9V→12V→15V→20V→28V）を自動切替する基本テストスケッチ。

## CH224Aライブラリ

`src/CH224A.h`および`src/CH224A.cpp`に再利用可能なライブラリクラスを提供。

### 使用例

```cpp
#include <Wire.h>
#include "src/CH224A.h"

CH224A ch224a;

void setup() {
  Wire.begin(21, 22);
  
  if (ch224a.begin()) {
    // 固定電圧設定
    ch224a.setVoltage9V();
    
    // PPS電圧設定（9.5V）
    ch224a.setPPSVoltage(9.5);
    
    // AVS電圧設定（12.0V）
    ch224a.setAVSVoltage(12.0);
  }
}
```

### API

| メソッド                          | 説明                           |
|-----------------------------------|--------------------------------|
| `begin(TwoWire &wire)`            | 初期化・接続確認               |
| `isConnected()`                   | I2C接続確認                    |
| `setVoltage5V()` ～ `setVoltage28V()` | 固定電圧設定               |
| `setFixedVoltage(CH224A_VoltageMode)` | 電圧モード設定             |
| `setPPSVoltage(float voltage)`    | PPS電圧設定（V単位）           |
| `setPPSVoltageRaw(uint8_t value)` | PPS電圧設定（0.1V単位）        |
| `setAVSVoltage(float voltage)`    | AVS電圧設定（V単位）           |
| `setAVSVoltageRaw(uint16_t value)`| AVS電圧設定（0.1V単位）        |
| `readRegister(reg, &value)`       | レジスタ読み込み               |
| `writeRegister(reg, value)`       | レジスタ書き込み               |
| `readSourceCapabilities(data, length)` | Source Capabilities読み取り |
| `getPDOCount(header)`             | PDO数取得                     |
| `getPDOType(pdo)`                 | PDOタイプ取得（0:Fixed, 1:Variable, 3:PPS） |
| `parseFixedPDO(pdo, voltage, current)` | Fixed PDO解析            |
| `parseVariablePDO(pdo, minV, maxV, current)` | Variable PDO解析      |
| `parsePPSPDO(pdo, minV, maxV, current)` | PPS PDO解析             |
| `decodeHeader(header)`           | PDOヘッダーデコード表示        |
| `decodePDOs(data, numPDO)`        | PDOデータデコード表示          |

## CH224A技術仕様

### 対応プロトコル

- USB PD3.2 EPR（Extended Power Range）
- USB PD PPS（Programmable Power Supply）
- USB PD AVS（Adjustable Voltage Supply）
- USB PD SPR（Standard Power Range）
- BC1.2

### 電気的仕様

- **入力電圧範囲**: 4V～30V
- **I2C通信速度**: 最大400kHz
- **I2Cアドレス**: 0x22（デフォルト）または0x23

### レジスタマップ

| アドレス    | 名称                | 説明                     |
|-------------|---------------------|--------------------------|
| 0x09        | I2C_STATUS          | I2C状態                  |
| 0x0A        | VOLTAGE_CTRL        | 電圧制御（0-7）          |
| 0x50        | CURRENT_DATA        | 電流データ（50mA単位）   |
| 0x51-0x52   | AVS_VOLTAGE         | AVS電圧（16ビット）      |
| 0x53        | PPS_VOLTAGE         | PPS電圧（0.1V単位）      |
| 0x60-0x7F   | SRCCAP_DATA         | Source Capabilities      |
| 0x80-0x8F   | EPR_SRCCAP_DATA     | EPR Source Capabilities  |

### 電圧制御レジスタ（0x0A）の値

| 値 | モード    |
|----|-----------|
| 0  | 5V固定    |
| 1  | 9V固定    |
| 2  | 12V固定   |
| 3  | 15V固定   |
| 4  | 20V固定   |
| 5  | 28V固定   |
| 6  | PPSモード |
| 7  | AVSモード |

## トラブルシューティング

### CH224Aが応答しない

1. SDA/SCLの配線を確認
2. I2Cアドレスを確認（`I2C_Scanner`スケッチを使用）
3. CFG2/CFG3ピンに抵抗が接続されていないか確認
4. 必要に応じて4.7kΩプルアップ抵抗を追加

### 電圧が変わらない

1. USB PD充電器がPPS/AVSをサポートしているか確認
2. USB PD対応ケーブルを使用しているか確認
3. 充電器がサポートする電圧範囲内で設定しているか確認

### 画面が表示されない

1. M5Unifiedライブラリが正しくインストールされているか確認
2. 適切なボードが選択されているか確認

## ライセンス

MITライセンス - 詳細は [LICENSE](LICENSE) を参照

## 参考資料

- [CH224Aデータシート](https://wch.cn)
- [M5Stack公式ドキュメント](https://docs.m5stack.com)
- [M5Unified GitHub](https://github.com/m5stack/M5Unified)
- [USB Power Delivery仕様](https://www.usb.org/usb-charger-pd)
