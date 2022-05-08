/////////////////////
//    Libraries    //
/////////////////////

#include <EEPROM.h>
#include <SPI.h>
#include <mcp2515.h> // https://github.com/autowp/arduino-mcp2515 + https://github.com/watterott/Arduino-Libs/tree/master/digitalWriteFast

/////////////////////
//  Configuration  //
/////////////////////

#define CS_PIN_CAN0 9
#define CS_PIN_CAN1 10
#define SERIAL_SPEED 115200
#define CAN_SPEED CAN_125KBPS // Diagnostic CAN bus - High Speed
#define CAN_FREQ MCP_16MHZ // Switch to 16MHZ if you have a 16Mhz module

////////////////////
// Initialization //
////////////////////

MCP2515 CAN0(CS_PIN_CAN0); // CAN-BUS Shield N°1
MCP2515 CAN1(CS_PIN_CAN1); // CAN-BUS Shield N°2

////////////////////
//   Variables    //
////////////////////

// My variables
bool debugCAN0 = false;
bool debugCAN1 = false;
bool SerialEnabled = false;
int modes = 0x00;
long lastmode;
bool ACCState = false;
int ACCTime = 0x00;
int Animation = 0x00;

// CAN-BUS Messages
struct can_frame canMsgSnd;
struct can_frame canMsgRcv;
char tmp[4];

void setup() {
  if (SerialEnabled) {
    // Initalize Serial for debug
    Serial.begin(SERIAL_SPEED);

    // CAN-BUS from car
    Serial.println("Initialization CAN0");
  }

  CAN0.reset();
  CAN0.setBitrate(CAN_SPEED, CAN_FREQ);
  while (CAN0.setNormalMode() != MCP2515::ERROR_OK) {
    delay(100);
    Serial.println("*");
  }

  if (SerialEnabled) {
    // CAN-BUS to CAN2010 device(s)
    Serial.println("Initialization CAN1");
  }

  CAN1.reset();
  CAN1.setBitrate(CAN_SPEED, CAN_FREQ);
  while (CAN1.setNormalMode() != MCP2515::ERROR_OK) {
    delay(100);
    Serial.println("*");
  }
  lastmode=millis();
}

void loop() {
  // Receive CAN messages from the car
  if (CAN0.readMessage( & canMsgRcv) == MCP2515::ERROR_OK) {
    int id = canMsgRcv.can_id;
    int len = canMsgRcv.can_dlc;

    if (debugCAN0) {
      Serial.print("FRAME:ID=");
      Serial.print(id);
      Serial.print(":LEN=");
      Serial.print(len);

      char tmp[3];
      for (int i = 0; i < len; i++) {
        Serial.print(":");

        snprintf(tmp, 3, "%02X", canMsgRcv.data[i]);

        Serial.print(tmp);
      }

      Serial.println();
    }
    if (id == 0x21F && len == 3 && canMsgRcv.data[2]==0x40){
      CAN1.sendMessage( & canMsgRcv);
      if (millis()-lastmode > 500){
        modes+=1;
        lastmode=millis();}
      }
    else if (id == 0x0A2){
        canMsgSnd.data[0] = canMsgRcv.data[0];
        canMsgSnd.data[1] = canMsgRcv.data[1];
        canMsgSnd.data[2] = canMsgRcv.data[2];
        canMsgSnd.data[3] = modes;
        canMsgSnd.data[4] = canMsgRcv.data[4];
        canMsgSnd.data[5] = canMsgRcv.data[5];
        canMsgSnd.can_id = 0x0A2;
        canMsgSnd.can_dlc = 6;
        CAN1.sendMessage( & canMsgSnd);
    }
    else if (id == 0x236){
      if (Animation == 0x00 && millis() > 5000) {
        canMsgSnd.data[0] = canMsgRcv.data[0];
        canMsgSnd.data[1] = canMsgRcv.data[1];
        canMsgSnd.data[2] = canMsgRcv.data[2];
        canMsgSnd.data[3] = canMsgRcv.data[3];
        canMsgSnd.data[4] = canMsgRcv.data[4];
        canMsgSnd.data[5] = bitWrite(canMsgRcv.data[5], 6, 1);
        canMsgSnd.data[6] = canMsgRcv.data[6];
        canMsgSnd.data[7] = canMsgRcv.data[7];
        canMsgSnd.can_id = 0x236;
        canMsgSnd.can_dlc = 8;
        CAN1.sendMessage( & canMsgSnd);
        Animation = 0x01;
      }
      else {CAN1.sendMessage( & canMsgRcv);}
    }
    else if (id == 0x1E9 && bitRead(canMsgRcv.data[2],3)== 1){
      canMsgSnd.data[0] = canMsgRcv.data[1];
      canMsgSnd.data[1] = 0x10;
      canMsgSnd.data[2] = 0x00;
      canMsgSnd.data[3] = 0x00;
      canMsgSnd.data[4] = 0x00;
      canMsgSnd.data[5] = 0x00;
      canMsgSnd.data[6] = 0x00;
      canMsgSnd.data[7] = 0x00;
      canMsgSnd.can_id = 0x268;
      canMsgSnd.can_dlc = 8;
      CAN1.sendMessage( & canMsgSnd);
      }
    else if (id == 0x2A8){
      CAN1.sendMessage( & canMsgRcv);
      if (bitRead(canMsgRcv.data[0],7)== 1){
       ACCState=true; 
      }
      else{ACCState=false;}
      }
    else if (id == 0x1E7){
      CAN1.sendMessage( & canMsgRcv);
      ACCTime = canMsgRcv.data[2];
      }
    else if (id == 0x228){
      if (ACCState == false){
        CAN1.sendMessage( & canMsgRcv);}
      else if(ACCState == true && bitRead(canMsgRcv.data[2],7)== 0 && bitRead(canMsgRcv.data[2],6)== 1){
        ACCTime = bitWrite(ACCTime, 7, bitRead(canMsgRcv.data[3],7));
        ACCTime = bitWrite(ACCTime, 6, bitRead(canMsgRcv.data[3],6));
        canMsgSnd.data[0] = canMsgRcv.data[0];
        canMsgSnd.data[1] = canMsgRcv.data[1];
        canMsgSnd.data[2] = bitWrite(canMsgRcv.data[2], 7, 1);
        canMsgSnd.data[3] = ACCTime;
        canMsgSnd.data[4] = canMsgRcv.data[4];
        canMsgSnd.data[5] = canMsgRcv.data[5];
        canMsgSnd.data[6] = canMsgRcv.data[6];
        canMsgSnd.data[7] = canMsgRcv.data[7];
        canMsgSnd.can_id = 0x228;
        canMsgSnd.can_dlc = 8;
        CAN1.sendMessage( & canMsgSnd);
        }
        else {
          CAN1.sendMessage( & canMsgRcv);
        }
      }
    else {    
      CAN1.sendMessage( & canMsgRcv);  
    }
  }

  

  // Forward messages from the CAN2010 device(s) to the car
  if (CAN1.readMessage( & canMsgRcv) == MCP2515::ERROR_OK) {
    int id = canMsgRcv.can_id;
    int len = canMsgRcv.can_dlc;

    if (debugCAN1) {
      Serial.print("FRAME:ID=");
      Serial.print(id);
      Serial.print(":LEN=");
      Serial.print(len);

      char tmp[3];
      for (int i = 0; i < len; i++) {
        Serial.print(":");

        snprintf(tmp, 3, "%02X", canMsgRcv.data[i]);

        Serial.print(tmp);
      }

      Serial.println();
    }
    CAN0.sendMessage( & canMsgRcv);
  }
}
