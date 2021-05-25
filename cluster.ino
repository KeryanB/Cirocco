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
#define CAN_FREQ MCP_8MHZ // Switch to 16MHZ if you have a 16Mhz module

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
int ESPPin = 8;
int ClusterPin =7;
int ESPLed = 3;
bool SerialEnabled = true;

int counter = 0;
long previousread;
long lastpushed;
long lastESP;
long lastCluster;
int modes = 0x00;

// CAN-BUS Messages
struct can_frame canMsgSnd;
struct can_frame canMsgRcv;
char tmp[4];

void setup() {
  pinMode(ESPPin, INPUT_PULLUP);
  pinMode(ClusterPin, INPUT_PULLUP);
  pinMode(ESPLed, OUTPUT);
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
}

void loop() {
  int tmpVal;
   if ((millis()-previousread)>1000){
    counter = 0;
    previousread=millis();
   }
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
    if (id == 0x227 && canMsgRcv.data[0]==0x10){
      digitalWrite(ESPLed,HIGH);
    Serial.print("LED ESP ON");
    }
    if (id == 0x227 && canMsgRcv.data[0]==0x00){
      digitalWrite(ESPLed,LOW);
    Serial.print("LED ESP OFF");
    }
    if (id == 0x0A2 && canMsgRcv.data[0]==0x00 && canMsgRcv.data[1]==0x00 && canMsgRcv.data[2]==0x00 && canMsgRcv.data[3]==0x00 && canMsgRcv.data[4]==0x00 && canMsgRcv.data[5]==0x00){
      Serial.print("0A2 not send");
    }
    else if (id == 0x221 && len == 7 && canMsgRcv.data[0]==0x08){
        counter +=1;
        Serial.print("counter+=1");
      if (counter >= 2){
        if (modes >= 0x08){
          modes == 0x01;
        }
        modes+=1;
        canMsgSnd.data[0] = 0x00;
        canMsgSnd.data[1] = 0x00;
        canMsgSnd.data[2] = 0x00;
        canMsgSnd.data[3] = modes;
        canMsgSnd.data[4] = 0x00;
        canMsgSnd.data[5] = 0x00;
        canMsgSnd.can_id = 0x0A2;
        canMsgSnd.can_dlc = 6;
        CAN1.sendMessage( & canMsgSnd);
        Serial.print("Mode changé");
      }
      else
      {CAN1.sendMessage( & canMsgRcv);}
      Serial.print("changement conso");
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
     if (millis()-lastESP > 500){
    if (digitalRead(ESPPin) == LOW){
        canMsgSnd.data[0] = 0x00;
        canMsgSnd.data[1] = 0x00;
        canMsgSnd.data[2] = 0x40;
        canMsgSnd.data[3] = 0x00;
        canMsgSnd.data[4] = 0x00;
        canMsgSnd.can_id = 0x217;
        canMsgSnd.can_dlc = 5;
        CAN1.sendMessage( & canMsgSnd);
        CAN0.sendMessage( & canMsgSnd);
        Serial.print("ESP Send");
      lastESP = millis();}
   }
   if (millis()-lastCluster > 500){
    if (digitalRead(ClusterPin) == LOW){
        if (modes >= 0x08){
          modes == 0x01;
        }
        modes+=1;
        canMsgSnd.data[0] = 0x00;
        canMsgSnd.data[1] = 0x00;
        canMsgSnd.data[2] = 0x00;
        canMsgSnd.data[3] = modes;
        canMsgSnd.data[4] = 0x00;
        canMsgSnd.data[5] = 0x00;
        canMsgSnd.can_id = 0x0A2;
        canMsgSnd.can_dlc = 6;
        CAN1.sendMessage( & canMsgSnd);
        Serial.print("Mode changé");
      lastCluster = millis();}
   }
}