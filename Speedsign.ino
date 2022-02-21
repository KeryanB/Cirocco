/////////////////////
//    Libraries    //
/////////////////////

#include <EEPROM.h>
#include <SPI.h>
#include <mcp2515.h> // https://github.com/autowp/arduino-mcp2515 + https://github.com/watterott/Arduino-Libs/tree/master/digitalWriteFast

/////////////////////
//  Configuration  //
/////////////////////

#define CS_PIN_CAN0 10
#define SERIAL_SPEED 115200
#define CAN_SPEED CAN_125KBPS // Diagnostic CAN bus - High Speed
#define CAN_FREQ MCP_8MHZ // Switch to 16MHZ if you have a 16Mhz module

////////////////////
// Initialization //
////////////////////

MCP2515 CAN0(CS_PIN_CAN0); // CAN-BUS Shield N°2

////////////////////
//   Variables    //
////////////////////

// My variables
bool debugCAN0 = false;
bool SerialEnabled = true;

// CAN-BUS Messages
struct can_frame canMsgSnd;
struct can_frame canMsgRcv;
char tmp[4];

void setup() {
  if (SerialEnabled) {
    // Initalize Serial for debug
    Serial.begin(SERIAL_SPEED);
    // CAN-BUS to CAN2010 device(s)
    Serial.println("Initialization CAN0");
  }

  CAN0.reset();
  CAN0.setBitrate(CAN_SPEED, CAN_FREQ);
  while (CAN0.setNormalMode() != MCP2515::ERROR_OK) {
    delay(100);
    Serial.println("*");
  }
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
    if (id == 0x1E9 && bitRead(canMsgRcv.data[2],3)== 1){
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
      CAN0.sendMessage( & canMsgSnd);
      }
    }
  }
