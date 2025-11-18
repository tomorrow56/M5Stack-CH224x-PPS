# M5Stack Core Basic - CH224A PPS Mode Controller

M5Stack Core BasicでCH224AチップをI2C経由で制御し、USB PD PPSモードで電圧を調整するArduinoスケッチです。

## 機能

- **固定電圧モード**: 5V、9V、12V、15V、20Vの固定電圧から選択
- **PPSモード**: 5.0V～21.0Vの範囲で0.2Vステップで電圧を調整
- **直感的なUI**: M5Stackの画面に現在の電圧とモードを表示
- **ボタン操作**: 3つのボタンで簡単に電圧とモードを切り替え

## ハードウェア接続

### 必要な部品
- M5Stack Core Basic
- CH224A USB PDチップ(ESSOP10またはDFN10パッケージ)
- ジャンパーワイヤー

### 接続方法

| M5Stack Core Basic | CH224A     | 説明           |
|--------------------|------------|----------------|
| GPIO21 (SDA)       | Pin 3 (CFG3/SDA) | I2C データ線 |
| GPIO22 (SCL)       | Pin 2 (CFG2/SCL) | I2C クロック線 |
| GND                | Pin 0 (GND)      | グランド      |

**注意**: CH224AのCFG2とCFG3ピンは、I2C通信を使用する場合、空きピン(フローティング)にする必要があります。単電阻配置を使用している場合は、抵抗を取り外してください。

### CH224Aの配線例

CH224AをI2Cモードで使用する場合の基本的な配線:

```
VBUS (USB PD入力) --- [1uF] --- VHV (Pin 1)
                              |
                             GND (Pin 0)

USB D+ --- DP (Pin 4)
USB D- --- DM (Pin 5)

Type-C CC1 --- CC1 (Pin 7)
Type-C CC2 --- CC2 (Pin 6)

VBUS (電圧検出) --- VBUS (Pin 8)

PG (Pin 10) --- [オプション] LED + 抵抗 --- GND
```

## ソフトウェア要件

### Arduino IDE設定

1. **Arduino IDEのインストール**: [Arduino公式サイト](https://www.arduino.cc/en/software)からダウンロード

2. **M5Stackボードマネージャの追加**:
   - Arduino IDE → ファイル → 環境設定
   - 「追加のボードマネージャのURL」に以下を追加:
     ```
     https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
     ```

3. **M5Stackボードのインストール**:
   - ツール → ボード → ボードマネージャ
   - "M5Stack"で検索してインストール

4. **M5Stackライブラリのインストール**:
   - スケッチ → ライブラリをインクルード → ライブラリを管理
   - "M5Stack"で検索してインストール

### ボード設定

- ボード: "M5Stack-Core-ESP32"
- Upload Speed: 921600
- Flash Frequency: 80MHz
- Partition Scheme: Default

## 使用方法

### スケッチのアップロード

1. M5Stack Core BasicをUSBケーブルでPCに接続
2. Arduino IDEで`M5Stack_CH224A_PPS.ino`を開く
3. ツール → シリアルポートで適切なポートを選択
4. アップロードボタンをクリック

### 操作方法

M5Stackの3つのボタンで操作します:

- **ボタンA(左)**: 電圧を下げる
  - 固定電圧モード: 前の電圧プリセットに切り替え(5V → 9V → 12V → 15V → 20V)
  - PPSモード: 0.2Vずつ電圧を下げる

- **ボタンB(中央)**: モード切替
  - 固定電圧モード ⇔ PPSモードを切り替え

- **ボタンC(右)**: 電圧を上げる
  - 固定電圧モード: 次の電圧プリセットに切り替え
  - PPSモード: 0.2Vずつ電圧を上げる

### 画面表示

- **上部**: タイトルバー(青色背景)
- **中央上**: 現在のモード(黄色)
- **中央**: 現在の電圧(緑色、大きく表示)
- **中央下**: PPSモードの場合、電圧範囲を表示(水色)
- **下部**: ボタンラベル(青色/オレンジ色)

## カスタマイズ

### PPS電圧範囲の変更

スケッチ内の以下の変数を変更することで、PPS電圧範囲をカスタマイズできます:

```cpp
int minPPSVoltage = 50;   // 最小PPS電圧(5.0V) - 0.1V単位
int maxPPSVoltage = 210;  // 最大PPS電圧(21.0V) - 0.1V単位
int ppsStep = 2;          // PPS電圧ステップ(0.2V) - 0.1V単位
```

### 固定電圧プリセットの変更

固定電圧プリセットを変更する場合:

```cpp
const int fixedVoltages[] = {5, 9, 12, 15, 20, 28};  // 28Vを追加
const int fixedVoltageCount = 6;  // カウントを更新
```

### I2Cアドレスの変更

CH224AのI2Cアドレスが0x23の場合:

```cpp
#define CH224A_ADDR 0x23
```

## トラブルシューティング

### CH224Aが応答しない

1. **I2C接続を確認**: SDA/SCLの配線が正しいか確認
2. **I2Cアドレスを確認**: CH224Aのアドレスが0x22または0x23か確認
3. **プルアップ抵抗**: I2C信号線に4.7kΩのプルアップ抵抗が必要な場合があります
4. **CFG2/CFG3ピン**: これらのピンが抵抗でGNDやVHVに接続されていないか確認

### 電圧が変わらない

1. **USB PD対応**: 接続しているUSB PD充電器がPPSをサポートしているか確認
2. **ケーブル**: USB PD対応のケーブルを使用しているか確認
3. **電圧範囲**: 充電器がサポートする電圧範囲内で設定しているか確認

### 画面が表示されない

1. **M5Stackライブラリ**: 最新版のM5Stackライブラリがインストールされているか確認
2. **ボード設定**: "M5Stack-Core-ESP32"が選択されているか確認

## 技術仕様

### CH224A仕様
- **対応プロトコル**: USB PD3.2 EPR、AVS、PPS、SPR、BC1.2
- **入力電圧範囲**: 4V～30V
- **I2C通信速度**: 400kHz
- **I2Cアドレス**: 0x22または0x23

### PPS電圧設定
- **レジスタ**: 0x53
- **単位**: 0.1V(100mV)
- **範囲**: 通常5.0V～21.0V(充電器による)

### 固定電圧設定
- **レジスタ**: 0x0A
- **値**:
  - 0: 5V
  - 1: 9V
  - 2: 12V
  - 3: 15V
  - 4: 20V
  - 5: 28V
  - 6: PPSモード
  - 7: AVSモード

## ライセンス

このプロジェクトはMITライセンスの下で公開されています。

## 参考資料

- [CH224Aデータシート](https://wch.cn)
- [M5Stack公式ドキュメント](https://docs.m5stack.com)
- [USB Power Delivery仕様](https://www.usb.org/usb-charger-pd)

## 作成者

Manus AI Agent

## バージョン履歴

- **v1.0.0** (2025-11-15): 初回リリース
  - 固定電圧モードとPPSモードの実装
  - M5Stack画面UIの実装
  - ボタン操作による電圧制御
