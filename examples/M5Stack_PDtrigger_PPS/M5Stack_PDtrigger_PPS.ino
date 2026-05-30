/***************************************************
 * M5Stack PD Trigger (CH224Q & ACS712xLCTR-05B)
 * 2023/08/26
 * Copyright(c) @tomorrow56 all rights reserved
 ****************************************************/

#include <M5Unified.h>
#include <Wire.h>
#include <CH224A.h>

#ifdef USE_M5STACK_UPDATER
#include "M5StackUpdater.h"
#endif

//fot calibration parameter measurement
//#define CALIBRATE

// M5Stack Pin config
// no PSRAM model only
#define VBUS_I          35
#define VI_I            36
#define CFG2_O          22
#define CFG3_O          21
#define VBUSEN_O         2
#define PG_I            12

const int sdaPin = CFG3_O;
const int sclPin = CFG2_O;

// M5Stack Core2 Pin config
/*
#define VBUS_I          35
#define VI_I            36
#define CFG2_O          14
#define CFG3_O          19
#define VBUSEN_O        32
#define PG_I            27
*/

/**********
* Calibration parameter
* Changed to match actual measurements with your equipment
**********/
float vScale = 7.5;  // actual VBUS/VBUS_I
float vi_0A = 2.44;  // VI_I(V) @ 0A output
float vi_2A = 2.85;  // VI_I(V) @ 2A output
float vi_0cal = vi_0A;
float   vbus_i_temp[20];  // for moving average

uint8_t PDO = 0;
bool OE = false;
bool PPS_CONTROL = false;
bool PDO_DECODE_MODE = false;
float PPS_MIN = 5.0;
float PPS_MAX = 20.0;
float currentPPSVoltage = 5.0;

// PDO list display variables
uint8_t availablePDOs[7];  // Available PDO list
uint8_t numAvailablePDOs = 0;
uint8_t selectedPDO = 0;   // Currently selected PDO in list
bool pdoListNeedsUpdate = true;  // Flag to update PDO list display
bool pdoModeSupported[6] = {true, true, true, true, true, false};

CH224A ch224a;

uint32_t updateTime = 0;       // time for next update
uint8_t interval = 100;  // Update interval

bool pdoDecodeFirstDraw = true;

const uint32_t PPS_HOLD_START_MS = 1000;
const uint32_t PPS_HOLD_REPEAT_MS = 120;
uint32_t ppsHoldStartA = 0;
uint32_t ppsHoldStartC = 0;
uint32_t ppsLastRepeatA = 0;
uint32_t ppsLastRepeatC = 0;

static void setMainTextColor() {
  if (OE) {
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
}

static bool updatePpsRangeFromPdo() {
  uint8_t data[48];
  if (!ch224a.readSourceCapabilities(data, sizeof(data))) {
    return false;
  }

  uint16_t header = data[0] | (data[1] << 8);
  uint8_t numPDO = ch224a.getPDOCount(header);

  bool foundPps = false;
  float foundMin = 999.0;
  float foundMax = 0.0;

  for (int i = 0; i < numPDO; i++) {
    uint32_t pdo = data[i * 4 + 2]
      | (data[i * 4 + 3] << 8)
      | (data[i * 4 + 4] << 16)
      | (data[i * 4 + 5] << 24);
    uint8_t pdoType = ch224a.getPDOType(pdo);
    if (pdoType != 3) {
      continue;
    }

    float minVoltage = 0.0;
    float maxVoltage = 0.0;
    float current = 0.0;
    ch224a.parsePPSPDO(pdo, minVoltage, maxVoltage, current);

    foundPps = true;
    if (minVoltage < foundMin) {
      foundMin = minVoltage;
    }
    if (maxVoltage > foundMax) {
      foundMax = maxVoltage;
    }
  }

  if (!foundPps) {
    return false;
  }

  if (foundMin >= 5.0) {
    PPS_MIN = foundMin;
  }
  if (foundMax <= 20.0) {
    PPS_MAX = foundMax;
  }
  if (PPS_MIN > PPS_MAX) {
    float tmp = PPS_MIN;
    PPS_MIN = PPS_MAX;
    PPS_MAX = tmp;
  }

  return true;
}

static void updateBtnLabels() {
  if (PDO_DECODE_MODE) {
    drawBtnMenu("", "", "EXIT");
    return;
  }
  if (PDO == 0 && !PPS_CONTROL) {
    drawBtnMenu("PDO", OE ? "OFF" : "ON", "UP");
    return;
  }
  if (PDO == 4 && !PPS_CONTROL) {
    drawBtnMenu("DOWN", OE ? "OFF" : "ON", pdoModeSupported[5] ? "PPS" : "");
    return;
  }
  drawBtnMenu("DOWN", OE ? "OFF" : "ON", "UP");
}

static void updateSelectedPDO() {
  selectedPDO = PDO;
  pdoListNeedsUpdate = true;
}

static bool applyPdoVoltage(uint8_t pdo) {
  switch (pdo) {
    case 0:
      return ch224a.setVoltage5V();
    case 1:
      return ch224a.setVoltage9V();
    case 2:
      return ch224a.setVoltage12V();
    case 3:
      return ch224a.setVoltage15V();
    case 4:
      return ch224a.setVoltage20V();
    case 5:
      return ch224a.setPPSVoltage(5.0);
    default:
      return ch224a.setVoltage5V();
  }
}

void setup(void) {
  // M5Stack::begin(LCDEnable, SDEnable, SerialEnable, I2CEnable);
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = false;
  M5.begin(cfg);

#ifdef USE_M5STACK_UPDATER
  if (M5.BtnA.isPressed()) {
    Serial.println("Will Load menu binary");
    updateFromFS(SD);
    ESP.restart();
  }
#endif

  // Pin initialize
  pinMode(VBUS_I, INPUT);
  pinMode(VI_I, INPUT);
  pinMode(PG_I, INPUT);

  pinMode(VBUSEN_O, OUTPUT);

  // I2C Initialize
  Wire.begin(sdaPin, sclPin);
  (void)ch224a.begin(Wire);
 
  // Force PDO=0 (5V) on startup
  PDO = 0;
  PPS_CONTROL = false;
  (void)applyPdoVoltage(PDO);
  // Output disable
  digitalWrite(VBUSEN_O, LOW); 
  OE = false;

  // M5.Display.setRotation(1);
  M5.Display.fillScreen(TFT_BLACK);
  drawTitle("M5 PD Analyzer");
  
  // Initialize available PDOs list
  updateAvailablePDOs();
  updateSelectedPDO();
  pdoListNeedsUpdate = true;
  
  updateBtnLabels();
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);

  for(int i = 0; i < 20; i++){
    vbus_i_temp[i] = readVoltage(analogRead(VI_I));
    delay(20);
  }

  vi_0cal = averageVI();

  updateTime = millis(); // Next update time
}

void loop() {
  float vbus_v;
  float vbus_i;
  char buf1[5];
  char buf2[5];

  M5.update();

  if (updateTime <= millis()) {
    updateTime = millis() + interval; // Update interval

    if(PDO_DECODE_MODE){
      // Draw once on entry (no periodic update to avoid flicker)
      if (pdoDecodeFirstDraw) {
        drawPDODecode();
      }
    } else {
      // Normal operation
      vbus_v = readVoltage(analogRead(VBUS_I)) * vScale;  // read data from CH1
      // 20 moving averages
      for(int i = 19; i > 0; i--){
        vbus_i_temp[i] = vbus_i_temp[i - 1];
      }
      vbus_i_temp[0]= readVoltage(analogRead(VI_I));    // read data from CH2
      vbus_i = (averageVI() - vi_0cal) / ((vi_2A - vi_0A) / 2);

      // Draw PDO list only when needed
      if(pdoListNeedsUpdate){
        drawPDOList();
        pdoListNeedsUpdate = false;
      }

#ifndef CALIBRATE
      dtostrf(vbus_v,4,1,buf1);
      drawText("Vout = " + (String)buf1 + " V    ", 1, 0);
      Serial.printf("Vout = %s V\n", buf1);

      dtostrf(vbus_i,5,2,buf2);
      drawText("Iout = " + (String)buf2 + " A    ", 1, 1);
      Serial.printf("Iout = %s A\n", buf2);
#else
      drawText("Vout(raw) = " + (String)readVoltage(analogRead(VBUS_I)) + " V    ", 1, 0);
      Serial.println("Vout(raw) = " + (String)readVoltage(analogRead(VBUS_I)) + " V");

      drawText("Iout(raw) = " + (String)readVoltage(analogRead(VI_I)) + " V    ", 1, 1);
      Serial.println("Iout(raw) = " + (String)readVoltage(analogRead(VI_I)) + " V");
#endif

      // Apply voltage only if not in PPS control mode
      if(!PPS_CONTROL) {
        (void)applyPdoVoltage(PDO);
      }

      switch(PDO){
        case 0:
        drawSubTitle("PDO 5V      ");
        break;
        case 1:
        drawSubTitle("PDO 9V      ");
        break;
        case 2:
        drawSubTitle("PDO 12V     ");
        break;
        case 3:
        drawSubTitle("PDO 15V     ");
        break;
        case 4:
        drawSubTitle("PDO 20V     ");
        break;
        case 5:
        if(PPS_CONTROL){
          char pps_buf[8];
          dtostrf(currentPPSVoltage, 4, 1, pps_buf);
          drawSubTitle("PPS " + (String)pps_buf + "V      ");
        } else {
          drawSubTitle("PPS MODE    ");
        }
        break;
        default:
        drawSubTitle("PDO 5V      ");
        break;
      }

      if(OE == true){
        digitalWrite(VBUSEN_O, HIGH);
        Serial.println("Output Enabled");
      }else{
        digitalWrite(VBUSEN_O, LOW); 
        Serial.println("Output Disabled");
        vi_0cal = averageVI();
      }
      
      drawPG(vbus_v, digitalRead(PG_I));
    }

  }

  if(M5.BtnA.wasPressed()){
    if(PPS_CONTROL){
      // PPS control mode - decrease voltage
      const float eps = 0.001;
      if(currentPPSVoltage > (PPS_MIN + eps)){
        currentPPSVoltage -= 0.1;
        if(currentPPSVoltage < PPS_MIN){
          currentPPSVoltage = PPS_MIN;
        }
        ch224a.setPPSVoltage(currentPPSVoltage);
      } else {
        // Exit PPS control mode
        PPS_CONTROL = false;
        PDO = 0;
        updateSelectedPDO();
        updateBtnLabels();
      }
    } else if(PDO_DECODE_MODE){
      // Navigate PDO list left
      if(selectedPDO > 0){
        selectedPDO--;
      } else {
        selectedPDO = numAvailablePDOs - 1;
      }
      pdoListNeedsUpdate = true;
    } else if(PDO == 0 && !PPS_CONTROL){
      // Enter PDO decode mode
      PDO_DECODE_MODE = true;
      pdoDecodeFirstDraw = true;
      updateAvailablePDOs();
      selectedPDO = 0;
      pdoListNeedsUpdate = true;
      updateBtnLabels();
    } else if(PDO > 0){
      PDO--;
      updateSelectedPDO();
      updateBtnLabels();
    }
  }
  
  if(M5.BtnB.wasPressed()){
    if(PDO_DECODE_MODE){
      // Do nothing in PDO decode mode - B button is disabled
    } else
    if(OE == false){
      OE = true;
      updateBtnLabels();
      setMainTextColor();
    }else{
      OE = false;
      updateBtnLabels();
      setMainTextColor();
    }
  }

  if(M5.BtnC.wasPressed()){
    if(PDO_DECODE_MODE){
      // Exit PDO decode mode without changing PDO
      PDO_DECODE_MODE = false;
      pdoDecodeFirstDraw = true;
      M5.Display.fillScreen(TFT_BLACK);
      drawTitle("M5 PD Analyzer");
      updateSelectedPDO();  // Restore selected PDO to match current PDO
      updateBtnLabels();
      M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    } else if(PDO == 5 && !PPS_CONTROL){
      // Enter PPS control mode
      if (pdoModeSupported[5] && updatePpsRangeFromPdo()) {
        PPS_CONTROL = true;
        currentPPSVoltage = PPS_MIN;
        updateBtnLabels();
      }
    } else if(PPS_CONTROL){
      // PPS control mode - increase voltage
      if(currentPPSVoltage < PPS_MAX){
        currentPPSVoltage += 0.1;
        ch224a.setPPSVoltage(currentPPSVoltage);
      }
    } else if(PDO < 5){
      if (PDO == 4 && !pdoModeSupported[5]) {
        // Keep 20V when PPS is not supported
      } else {
        PDO++;
        updateSelectedPDO();
        updateBtnLabels();
      }
    }
  }

  if (PPS_CONTROL && !PDO_DECODE_MODE) {
    const float eps = 0.001;
    uint32_t now = millis();

    if (M5.BtnA.isPressed()) {
      if (ppsHoldStartA == 0) {
        ppsHoldStartA = now;
        ppsLastRepeatA = now;
      } else if ((now - ppsHoldStartA) >= PPS_HOLD_START_MS
        && (now - ppsLastRepeatA) >= PPS_HOLD_REPEAT_MS) {
        if (currentPPSVoltage > (PPS_MIN + eps)) {
          currentPPSVoltage -= 0.1;
          if (currentPPSVoltage < PPS_MIN) {
            currentPPSVoltage = PPS_MIN;
          }
          ch224a.setPPSVoltage(currentPPSVoltage);
        }
        ppsLastRepeatA = now;
      }
    } else {
      ppsHoldStartA = 0;
      ppsLastRepeatA = 0;
    }

    if (M5.BtnC.isPressed()) {
      if (ppsHoldStartC == 0) {
        ppsHoldStartC = now;
        ppsLastRepeatC = now;
      } else if ((now - ppsHoldStartC) >= PPS_HOLD_START_MS
        && (now - ppsLastRepeatC) >= PPS_HOLD_REPEAT_MS) {
        if (currentPPSVoltage < (PPS_MAX - eps)) {
          currentPPSVoltage += 0.1;
          if (currentPPSVoltage > PPS_MAX) {
            currentPPSVoltage = PPS_MAX;
          }
          ch224a.setPPSVoltage(currentPPSVoltage);
        }
        ppsLastRepeatC = now;
      }
    } else {
      ppsHoldStartC = 0;
      ppsLastRepeatC = 0;
    }
  } else {
    ppsHoldStartA = 0;
    ppsHoldStartC = 0;
    ppsLastRepeatA = 0;
    ppsLastRepeatC = 0;
  }

}

void drawTitle(String Title){
  M5.Display.setTextSize(1);
  M5.Display.fillRect(0, 0, 320, 30, TFT_BLUE);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLUE);
  M5.Display.drawCentreString(Title, 160, 2, 4);
}

void drawSubTitle(String SubTitle){
  M5.Display.setTextSize(1);
  setMainTextColor();
  M5.Display.drawString(SubTitle, 10, 33, 4);
}

void drawText(String Text, int xPos, int yPos){
  M5.Display.setTextSize(1);
  setMainTextColor();
  M5.Display.drawString(Text, xPos * 30, yPos * 30 + 60 + 3, 4);
}

void drawBtnMenu(String A, String B, String C){
  M5.Display.setTextSize(1);
  M5.Display.fillRect(0, 210, 320, 30, TFT_BLUE);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLUE);
  M5.Display.drawCentreString(A, 65, 214, 4);
  M5.Display.drawCentreString(B, 160, 214, 4);
  M5.Display.drawCentreString(C, 255, 214, 4);
}

void drawPG(float vbus_v, bool PG){
  M5.Display.setTextSize(1);
  setMainTextColor();
  M5.Display.drawString("PG", 280, 30+ 3, 2);
  if(vbus_v >= 3){
    if(PG == LOW){
      M5.Display.fillRect(300, 30 + 5, 15, 15, TFT_GREEN);
        Serial.println("PG OK");
      }else{
        M5.Display.fillRect(300, 30 + 5, 15, 15, TFT_RED);
        Serial.println("PG NG");
      }
    }else{
      M5.Display.fillRect(300, 30 + 5, 15, 15, TFT_BLACK);
      M5.Display.drawRect(300, 30 + 5, 15, 15, TFT_WHITE);
    }
}

void drawPDODecode(){
  const int panelX = 20;
  const int panelY = 46;
  const int panelW = 280;
  const int panelH = 156;
  const int headerH = 24;

  if(pdoDecodeFirstDraw){
    M5.Display.fillScreen(TFT_BLACK);
    drawTitle("PDO Decode");
    drawBtnMenu("", "", "EXIT");

    // Sub-window style panel (shadow + border + header)
    M5.Display.fillRect(panelX + 3, panelY + 3, panelW, panelH, TFT_DARKGREY);
    M5.Display.fillRect(panelX, panelY, panelW, panelH, TFT_BLACK);
    M5.Display.drawRect(panelX, panelY, panelW, panelH, TFT_CYAN);
    M5.Display.fillRect(panelX + 2, panelY + 2, panelW - 4, headerH, TFT_BLUE);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLUE);
    M5.Display.drawCentreString("Source Capabilities", panelX + panelW / 2, panelY + 4, 2);

    pdoDecodeFirstDraw = false;
  }

  // Clear only sub-window content area
  M5.Display.fillRect(panelX + 4, panelY + headerH + 4, panelW - 8, panelH - headerH - 8, TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  
  // Read and decode PDO data
  uint8_t data[48]; // Enough for all PDO data
  if(ch224a.readSourceCapabilities(data, sizeof(data))){
    uint16_t header = data[0] | (data[1] << 8);
    uint8_t numPDO = ch224a.getPDOCount(header);

    // Display each PDO (up to 7 PDOs)
    for(int i = 0; i < numPDO && i < 7; i++){
      uint32_t pdo = data[i*4 + 2] | (data[i*4 + 3] << 8) | (data[i*4 + 4] << 16) | (data[i*4 + 5] << 24);
      uint8_t pdoType = ch224a.getPDOType(pdo);
      
      String pdoInfo = "";
      
      switch(pdoType){
        case 0: // Fixed Supply
          {
            float voltage, current;
            ch224a.parseFixedPDO(pdo, voltage, current);
            pdoInfo = "Fixed: " + String(voltage, 2) + "V " + String(current, 2) + "A";
          }
          break;
        case 1: // Variable Supply
          {
            float minVoltage, maxVoltage, current;
            ch224a.parseVariablePDO(pdo, minVoltage, maxVoltage, current);
            pdoInfo = "Var: " + String(minVoltage, 2) + "-" + String(maxVoltage, 2) + "V";
          }
          break;
        case 3: // PPS
          {
            float minVoltage, maxVoltage, current;
            ch224a.parsePPSPDO(pdo, minVoltage, maxVoltage, current);
            pdoInfo = "PPS: " + String(minVoltage, 2) + "-" + String(maxVoltage, 2) + "V " + String(current, 2) + "A";
          }
          break;
        default:
          pdoInfo = "Unknown";
          break;
      }
      M5.Display.drawString(pdoInfo, panelX + 12, panelY + headerH + 10 + i * 16, 2);
    }
  } else {
    M5.Display.drawCentreString("Read Error", panelX + panelW / 2, panelY + headerH + 10, 2);
  }
}

void updateAvailablePDOs() {
  uint8_t data[48];
  numAvailablePDOs = 0;
  for (int i = 0; i < 6; i++) {
    pdoModeSupported[i] = false;
  }
  
  if(ch224a.readSourceCapabilities(data, sizeof(data))){
    uint16_t header = data[0] | (data[1] << 8);
    uint8_t numPDO = ch224a.getPDOCount(header);
    
    // Add available PDOs to list (only fixed voltages 5V-20V)
    for(int i = 0; i < numPDO && i < 7; i++){
      uint32_t pdo = data[i*4 + 2] | (data[i*4 + 3] << 8) | (data[i*4 + 4] << 16) | (data[i*4 + 5] << 24);
      uint8_t pdoType = ch224a.getPDOType(pdo);
      
      if(pdoType == 0) { // Fixed Supply
        float voltage, current;
        ch224a.parseFixedPDO(pdo, voltage, current);

        uint8_t mode = 255;
        if (voltage >= 4.5 && voltage < 6.0) {
          mode = 0;
        } else if (voltage >= 8.5 && voltage < 10.0) {
          mode = 1;
        } else if (voltage >= 11.5 && voltage < 13.0) {
          mode = 2;
        } else if (voltage >= 14.5 && voltage < 16.0) {
          mode = 3;
        } else if (voltage >= 19.0 && voltage < 21.0) {
          mode = 4;
        }

        if (mode < 5) {
          pdoModeSupported[mode] = true;
          bool exists = false;
          for (int j = 0; j < numAvailablePDOs; j++) {
            if (availablePDOs[j] == mode) {
              exists = true;
              break;
            }
          }
          if (!exists && numAvailablePDOs < 7) {
            availablePDOs[numAvailablePDOs++] = mode;
          }
        }
      } else if (pdoType == 3) { // PPS
        pdoModeSupported[5] = true;
        bool exists = false;
        for (int j = 0; j < numAvailablePDOs; j++) {
          if (availablePDOs[j] == 5) {
            exists = true;
            break;
          }
        }
        if (!exists && numAvailablePDOs < 7) {
          availablePDOs[numAvailablePDOs++] = 5;
        }
      }
    }
  }
}

void drawPDOList() {
  // Clear PDO list area completely
  M5.Display.fillRect(0, 175, 320, 30, TFT_BLACK);

  const char* labels[6] = {"5V", "9V", "12V", "15V", "20V", "PPS"};
  int listStartX = 20;
  int listWidth = 280;
  int itemWidth = listWidth / 6;

  for(int i = 0; i < 6; i++){
    bool isSelected = (PDO == i);
    bool isSupported = pdoModeSupported[i];
    String pdoText = String(labels[i]);
    
    // Calculate text position
    int textX = listStartX + i * itemWidth + itemWidth/2;
    int textY = 183;  // Moved up by half character
    
    // Draw tight highlight box only for selected item
    if(isSelected){
      int boxW = 34;
      int boxH = 18;
      int maxW = itemWidth - 8;
      if (boxW > maxW) {
        boxW = maxW;
      }
      int boxX = textX - (boxW / 2);
      int boxY = textY - 2;
      M5.Display.fillRect(boxX, boxY, boxW, boxH, TFT_CYAN);
      M5.Display.setTextColor(TFT_BLACK);
    } else {
      M5.Display.setTextColor(isSupported ? TFT_WHITE : TFT_LIGHTGREY);
    }
    
    // Draw text with smaller font and center alignment
    M5.Display.setTextSize(1);
    M5.Display.drawCentreString(pdoText, textX, textY, 2);  // Font size 2 for smaller text
  }
}

float readVoltage(uint16_t Vread){
  float Vdc;
  // Convert the read data into voltage
  if(Vread < 5){
    Vdc = 0;
  }else if(Vread <= 1084){
    Vdc = 0.11 + (0.89 / 1084) * Vread;
  }else if(Vread <= 2303){
    Vdc = 1.0 + (1.0 / (2303 - 1084)) * (Vread - 1084);
  }else if(Vread <= 3179){
    Vdc = 2.0 + (0.7 / (3179 - 2303)) * (Vread - 2303);
  }else if(Vread <= 3659){
    Vdc = 2.7 + (0.3 / (3659 - 3179)) * (Vread - 3179);
  }else if(Vread <= 4071){
    Vdc = 3.0 + (0.2 / (4071 - 3659)) * (Vread - 3659);
  }else{
    Vdc = 3.2;
  }
  return Vdc;
}

float averageVI(){
  float vi_sum = 0;
  for(int i = 0; i < 20; i++){
    vi_sum = vi_sum + vbus_i_temp[i];
  }
  return vi_sum / 20;
}