# M5Stack-CH224x-PPS

An Arduino library and sample sketch collection for controlling CH224A/CH224Q USB PD controllers via I2C on M5Stack (M5Unified compatible), with USB PD PPS/AVS mode voltage adjustment.

## Features

- **Fixed Voltage Mode**: Select from 5V/9V/12V/15V/20V/28V fixed voltages
- **PPS Mode**: Adjust voltage in 0.1V steps within 5.0V～21.0V range
- **AVS Mode**: Variable voltage setting (16-bit precision)
- **Source Capabilities Analysis**: Detailed display of SRCCAP/EPR_SRCCAP messages
- **PDO Type Identification**: Automatic detection of Fixed/Battery/Variable/PPS Supply
- **CH224A Library**: Reusable library class (`src/CH224A.h`)

## Project Structure

```text
M5Stack-CH224x-PPS/
├── src/
│   ├── CH224A.h          # CH224A library header
│   └── CH224A.cpp        # CH224A library implementation
├── examples/
│   ├── M5Stack_PDtrigger_PPS/             # PD Analyzer with PDO decode
│   └── test/                              # Test and debug sketches
│       ├── M5Stack_CH224A_PPS/            # PPS voltage control (with GUI)
│       ├── CH224A_PDO_Test/               # PDO decode test (serial output)
│       ├── I2C_Scanner/                   # I2C device scanner
│       ├── PPS_Test/                      # PPS voltage sweep test
│       └── Simple_Test/                   # Fixed voltage auto-switch test
├── kicad/                                 # KiCad design files
│   └── M5_PDtrigger_PPS_v2/               # Hardware design v2
├── README_JP.md           # Japanese documentation
├── README.md             # This file
├── WIRING.md             # Detailed wiring information
└── LICENSE               # MIT license
```

## Hardware Requirements

### Required Components

- **M5Stack** (M5Unified compatible device: Core Basic, Core2, CoreS3, etc.)
- **CH224A/CH224K** USB PD controller (ESSOP10/DFN10 package)
- **USB Type-C connector**
- **1μF ceramic capacitor** (between VHV-GND)

### Pin Connections

| M5Stack       | CH224A          | Description              |
|---------------|-----------------|--------------------------|
| GPIO21        | Pin 3 (CFG3/SDA)| I2C data line           |
| GPIO22        | Pin 2 (CFG2/SCL)| I2C clock line          |
| GND           | Pin 0 (GND)     | Ground                  |
| GPIO2 (※)    | VBUSEN          | Output enable           |
| GPIO12 (※)   | Pin 10 (PG)     | Power Good input        |

※ Used in sample sketches. Can be changed according to your purpose.

**Important**: When using CFG2/CFG3 pins as I2C, do not connect resistors to these pins (leave them floating).

For detailed wiring information, see [WIRING.md](WIRING.md).

## Software Requirements

### Arduino IDE Setup

1. **Add M5Stack Board Manager**:
   - Arduino IDE → File → Preferences
   - Add to "Additional Board Manager URLs":
     ```text
     https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
     ```

2. **Install M5Stack Board**:
   - Tools → Board → Boards Manager
   - Search for "M5Stack" and install

3. **Install M5Unified Library**:
   - Sketch → Include Library → Manage Libraries
   - Search for "M5Unified" and install

### Board Settings

- **Board**: M5Stack-Core-ESP32 (or your M5Stack device)
- **Upload Speed**: 921600
- **Flash Frequency**: 80MHz

## Sample Sketches

### M5Stack_PDtrigger_PPS

M5Stack PD Analyzer - Main application for USB PD charger analysis and voltage control.

**Key Features**:

- Fixed voltage mode switching (5V/9V/12V/15V/20V)
- PPS mode voltage adjustment (0.1V steps)
- PDO Decode screen for detailed Source Capabilities display
- Real-time voltage/current measurement and PG (Power Good) display

**Operation**:

- **Button A (Left)**: Decrease voltage / Enter PDO Decode at PDO=0
- **Button B (Center)**: Output ON/OFF toggle
- **Button C (Right)**: Increase voltage / Enter PPS control at PDO=4 / Exit from PDO Decode

### M5Stack_CH224A_PPS

Legacy version of PPS voltage control GUI application.

### test/ Folder

Contains the following test and debug sketches.

#### CH224A_PDO_Test
Debug sketch that outputs PDO data to serial monitor. Displays raw data and parsed results of Source Capabilities.

#### I2C_Scanner
Scans I2C bus and displays device addresses. Used for CH224A connection verification.

#### PPS_Test
Test sketch that automatically sweeps PPS voltage from 5.5V to 20.0V in 0.5V steps.

#### Simple_Test
Basic test sketch that automatically switches fixed voltages (5V→9V→12V→15V→20V→28V) every 2 seconds.

## CH224A Library

Provides a reusable library class in `src/CH224A.h` and `src/CH224A.cpp`.

### Usage Example

```cpp
#include <Wire.h>
#include "src/CH224A.h"

CH224A ch224a;

void setup() {
  Wire.begin(21, 22);
  
  if (ch224a.begin()) {
    // Fixed voltage setting
    ch224a.setVoltage9V();
    
    // PPS voltage setting (9.5V)
    ch224a.setPPSVoltage(9.5);
    
    // AVS voltage setting (12.0V)
    ch224a.setAVSVoltage(12.0);
  }
}
```

### API

| Method                          | Description                           |
|-----------------------------------|---------------------------------------|
| `begin(TwoWire &wire)`            | Initialize and check connection       |
| `isConnected()`                   | Check I2C connection                  |
| `setVoltage5V()` ～ `setVoltage28V()` | Fixed voltage setting            |
| `setFixedVoltage(CH224A_VoltageMode)` | Voltage mode setting             |
| `setPPSVoltage(float voltage)`    | PPS voltage setting (in volts)        |
| `setPPSVoltageRaw(uint8_t value)` | PPS voltage setting (0.1V units)   |
| `setAVSVoltage(float voltage)`    | AVS voltage setting (in volts)        |
| `setAVSVoltageRaw(uint16_t value)`| AVS voltage setting (0.1V units)   |
| `readRegister(reg, &value)`       | Register read                         |
| `writeRegister(reg, value)`       | Register write                        |
| `readSourceCapabilities(data, length)` | Read Source Capabilities      |
| `getPDOCount(header)`             | Get PDO count                         |
| `getPDOType(pdo)`                 | Get PDO type (0:Fixed, 1:Variable, 3:PPS) |
| `parseFixedPDO(pdo, voltage, current)` | Parse Fixed PDO               |
| `parseVariablePDO(pdo, minV, maxV, current)` | Parse Variable PDO         |
| `parsePPSPDO(pdo, minV, maxV, current)` | Parse PPS PDO                  |
| `decodeHeader(header)`           | Decode and display PDO header         |
| `decodePDOs(data, numPDO)`        | Decode and display PDO data           |

## CH224A Technical Specifications

### Supported Protocols

- USB PD3.2 EPR (Extended Power Range)
- USB PD PPS (Programmable Power Supply)
- USB PD AVS (Adjustable Voltage Supply)
- USB PD SPR (Standard Power Range)
- BC1.2

### Electrical Specifications

- **Input Voltage Range**: 4V～30V
- **I2C Communication Speed**: Max 400kHz
- **I2C Address**: 0x22 (default) or 0x23

### Register Map

| Address    | Name                | Description                     |
|------------|---------------------|----------------------------------|
| 0x09       | I2C_STATUS          | I2C status                      |
| 0x0A       | VOLTAGE_CTRL        | Voltage control (0-7)           |
| 0x50       | CURRENT_DATA        | Current data (50mA units)       |
| 0x51-0x52  | AVS_VOLTAGE         | AVS voltage (16-bit)             |
| 0x53       | PPS_VOLTAGE         | PPS voltage (0.1V units)         |
| 0x60-0x7F  | SRCCAP_DATA         | Source Capabilities             |
| 0x80-0x8F  | EPR_SRCCAP_DATA     | EPR Source Capabilities         |

### Voltage Control Register (0x0A) Values

| Value | Mode       |
|-------|------------|
| 0     | 5V Fixed   |
| 1     | 9V Fixed   |
| 2     | 12V Fixed  |
| 3     | 15V Fixed  |
| 4     | 20V Fixed  |
| 5     | 28V Fixed  |
| 6     | PPS Mode   |
| 7     | AVS Mode   |

## Troubleshooting

### CH224A Not Responding

1. Check SDA/SCL wiring
2. Verify I2C address (use `I2C_Scanner` sketch)
3. Ensure no resistors are connected to CFG2/CFG3 pins
4. Add 4.7kΩ pull-up resistors if necessary

### Voltage Not Changing

1. Verify that the USB PD charger supports PPS/AVS
2. Use a USB PD compatible cable
3. Ensure settings are within the charger's supported voltage range

### Screen Not Displaying

1. Verify M5Unified library is properly installed
2. Check that the correct board is selected

## License

MIT License - see [LICENSE](LICENSE) for details

## References

- [CH224A Datasheet](https://wch.cn)
- [M5Stack Official Documentation](https://docs.m5stack.com)
- [M5Unified GitHub](https://github.com/m5stack/M5Unified)
- [USB Power Delivery Specification](https://www.usb.org/usb-charger-pd)
