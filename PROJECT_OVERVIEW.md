# プロジェクト概要

## M5Stack Core Basic - CH224A PPS Mode Controller

このプロジェクトは、M5Stack Core BasicでCH224A USB PDチップをI2C経由で制御し、USB Power Delivery(PD)のProgrammable Power Supply(PPS)モードで電圧を調整するArduinoスケッチです。

## プロジェクト構成

```
M5Stack_CH224A_PPS/
├── M5Stack_CH224A_PPS.ino          # メインスケッチ(直接I2C制御版)
├── M5Stack_CH224A_PPS_Library.ino  # メインスケッチ(ライブラリ使用版)
├── CH224A.h                         # CH224Aライブラリヘッダー
├── CH224A.cpp                       # CH224Aライブラリ実装
├── README.md                        # プロジェクトREADME
├── WIRING.md                        # 配線ガイド
├── LICENSE                          # MITライセンス
└── examples/                        # サンプルスケッチ
    ├── I2C_Scanner/                 # I2Cスキャナー
    │   └── I2C_Scanner.ino
    ├── Simple_Test/                 # シンプルテスト
    │   └── Simple_Test.ino
    └── PPS_Test/                    # PPSテスト
        └── PPS_Test.ino
```

## ファイル説明

### メインスケッチ

#### M5Stack_CH224A_PPS.ino
- **説明**: 直接I2C通信でCH224Aを制御するメインスケッチ
- **特徴**: 
  - 外部ライブラリ不要
  - シンプルな実装
  - 固定電圧モードとPPSモードの切り替え
  - M5Stackの3ボタンで操作
- **推奨用途**: 基本的な使用、学習目的

#### M5Stack_CH224A_PPS_Library.ino
- **説明**: CH224Aライブラリを使用した改良版スケッチ
- **特徴**:
  - オブジェクト指向設計
  - エラーハンドリング機能
  - コードの可読性が高い
  - 拡張性が高い
- **推奨用途**: 本格的な開発、カスタマイズ

### ライブラリファイル

#### CH224A.h / CH224A.cpp
- **説明**: CH224A制御用のC++ライブラリ
- **機能**:
  - 固定電圧設定(5V/9V/12V/15V/20V/28V)
  - PPS電圧設定(0.1V単位)
  - AVS電圧設定(0.1V単位)
  - I2C通信エラーチェック
  - レジスタ読み書き機能

### ドキュメント

#### README.md
- プロジェクトの概要
- ハードウェア接続方法
- ソフトウェア要件
- 使用方法
- カスタマイズ方法
- トラブルシューティング

#### WIRING.md
- 詳細な配線ガイド
- ピン配置図
- 配線図
- 配線手順
- 配線チェックリスト

### サンプルスケッチ

#### examples/I2C_Scanner/I2C_Scanner.ino
- **説明**: I2Cバス上のデバイスをスキャン
- **用途**: CH224AのI2Cアドレスを確認

#### examples/Simple_Test/Simple_Test.ino
- **説明**: 5秒ごとに電圧を自動切り替え
- **用途**: CH224Aの基本動作確認

#### examples/PPS_Test/PPS_Test.ino
- **説明**: PPSモードで電圧を自動スイープ
- **用途**: PPSモードの動作確認

## 主な機能

### 1. 固定電圧モード
- 5V、9V、12V、15V、20Vの固定電圧から選択
- ボタンA/Cで電圧を切り替え

### 2. PPSモード
- 5.0V～21.0Vの範囲で電圧を調整
- 0.2Vステップで細かく設定可能
- ボタンA/Cで電圧を上下

### 3. 直感的なUI
- 現在の電圧を大きく表示
- モード表示(固定電圧/PPS)
- 電圧範囲表示(PPSモード時)
- ボタンラベル表示

### 4. エラーハンドリング
- I2C通信エラー検出
- CH224A未接続検出
- 電圧設定失敗通知

## 技術仕様

### ハードウェア
- **マイコン**: M5Stack Core Basic(ESP32ベース)
- **PDチップ**: CH224A(ESSOP10/DFN10パッケージ)
- **通信**: I2C(SDA: GPIO21, SCL: GPIO22)
- **I2Cアドレス**: 0x22(デフォルト)または0x23

### ソフトウェア
- **開発環境**: Arduino IDE
- **必要ライブラリ**: M5Stack、Wire(標準)
- **対応ボード**: M5Stack-Core-ESP32

### CH224A仕様
- **対応プロトコル**: USB PD3.2 EPR、AVS、PPS、SPR、BC1.2
- **入力電圧範囲**: 4V～30V
- **I2C通信速度**: 400kHz
- **PPS電圧範囲**: 通常5.0V～21.0V(充電器による)
- **PPS電圧分解能**: 0.1V(100mV)

## 使用方法

### 基本的な使い方

1. **ハードウェア接続**: WIRING.mdを参照してM5StackとCH224Aを接続
2. **スケッチアップロード**: Arduino IDEで`M5Stack_CH224A_PPS.ino`を開いてアップロード
3. **操作**:
   - ボタンA: 電圧を下げる
   - ボタンB: モード切替(固定電圧⇔PPS)
   - ボタンC: 電圧を上げる

### 推奨ワークフロー

1. **I2Cスキャン**: `examples/I2C_Scanner`でCH224Aのアドレスを確認
2. **基本テスト**: `examples/Simple_Test`で固定電圧モードの動作確認
3. **PPSテスト**: `examples/PPS_Test`でPPSモードの動作確認
4. **本格使用**: `M5Stack_CH224A_PPS_Library.ino`で実用

## カスタマイズ

### 電圧範囲の変更

スケッチ内の以下の変数を変更:

```cpp
float minPPSVoltage = 5.0;   // 最小PPS電圧
float maxPPSVoltage = 21.0;  // 最大PPS電圧
float ppsStep = 0.2;         // 電圧ステップ
```

### 固定電圧プリセットの追加

```cpp
const float fixedVoltages[] = {5.0, 9.0, 12.0, 15.0, 20.0, 28.0};  // 28V追加
const int fixedVoltageCount = 6;
```

### I2Cアドレスの変更

```cpp
#define CH224A_ADDR 0x23  // 0x23に変更
```

または

```cpp
CH224A ch224a(CH224A_I2C_ADDR_ALT);  // ライブラリ版
```

## トラブルシューティング

### よくある問題

1. **CH224Aが応答しない**
   - I2C配線を確認
   - I2Cアドレスを確認(I2C_Scannerを使用)
   - プルアップ抵抗の追加を検討

2. **電圧が変わらない**
   - USB PD充電器がPPSをサポートしているか確認
   - USB Type-Cケーブルがフル機能対応か確認
   - CC1/CC2ピンの配線を確認

3. **画面が表示されない**
   - M5Stackライブラリが正しくインストールされているか確認
   - ボード設定が"M5Stack-Core-ESP32"になっているか確認

詳細はREADME.mdのトラブルシューティングセクションを参照してください。

## ライセンス

このプロジェクトはMITライセンスの下で公開されています。詳細はLICENSEファイルを参照してください。

## 参考資料

- [CH224Aデータシート](https://wch.cn)
- [M5Stack公式ドキュメント](https://docs.m5stack.com)
- [USB Power Delivery仕様](https://www.usb.org/usb-charger-pd)
- [Arduino Wire Library](https://www.arduino.cc/en/Reference/Wire)

## 作成者

Manus AI Agent

## バージョン

v1.0.0 (2025-11-15)

## 今後の拡張案

- [ ] 電圧・電流のリアルタイム表示
- [ ] 電圧プリセットの保存機能(EEPROM)
- [ ] Wi-Fi経由でのリモート制御
- [ ] グラフ表示機能
- [ ] 複数のCH224Aデバイスの制御
- [ ] AVSモードの実装
- [ ] PDデータレジスタの読み取り機能
