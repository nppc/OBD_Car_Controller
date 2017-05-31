#include <Arduino.h>
#include "definitions.h"

void resetBT() {
 digitalWrite(BT_ResetPin, LOW);
 delay(1000);
 digitalWrite(BT_ResetPin, HIGH);
 delay(2000);
}

void ATmodeBT() {
 digitalWrite(BT_PIO11Pin, HIGH);
 resetBT();
 Serial.begin(38400);  // must be 38400 because on this speed we will configure HC-05 
}

void CMDmodeBT() {
 digitalWrite(BT_PIO11Pin, LOW);
 resetBT();
 Serial.begin(9600);  // but this is the working speed of OBD BT receiver
 Serial.flush();
}

void setBTmaster() {
  ATmodeBT();
  sendATCommandBT("ROLE=1");
  sendATCommandBT("BIND=000D,18,A03579");   //OBDII addres: 00:0D:18:A0:35:79
  sendATCommandBT("CMODE=0");
  CMDmodeBT();
  OBDstatusTFTicon=0;  // master, icon must be OBD
}
void setBTslave() {
  ATmodeBT();
  sendATCommandBT("ROLE=0");
  sendATCommandBT("CMODE=1");
  CMDmodeBT();
  OBDstatusTFTicon=255;  // slave, icon must be different (BT to external device)
  updateOBDiconTFT();

}

void sendATCommandBT(char *command)
{
 Serial.print("AT+");
 Serial.print(command);
 Serial.print("\r\n");
 delay(100);
}

const char PROGMEM str_ser_asterix[] = "**";  // this symbols indicate a command
#define ser_command_len 10
const char PROGMEM str_ser_commands[][ser_command_len] = {  // every line is 9 chars max
  "DONE",      // 1 - indicates finish of receiving long term (multi byte) command
  "CHIPI2C",   // 2 - set an I2C chip address
  "ADDRI2C",   // 3 - set address inside I2C chip
  "SNDI2CDAT", // 4 - send series of bytes (until DONE command received) to the EEPROM via BT
  "SNDI2CU8",  // 5 - send byte to the EEPROM via BT
  "SNDI2CU16", // 6 - send word to the EEPROM via BT
  "GETI2CDAT", // 7 - request n bytes from the current adderss of EEPROM. After a command must supply a quantity of bytes (max 65535).
  "END"        // last command in list
};

//#define serTxtBuffLen 13  // can be not bigger than longest command

byte serCommand;  // 0 means no command received
byte serI2CchipPageMask;  // Mask for the check for page write to the  EEPROM
unsigned int serI2Caddr=0;

void readSerial(){
  serCommand=0;
  //char serBuff[serTxtBuffLen]; // Global bugger can be used
  Tft.Fcolor=YELLOW;Tft.X=0;Tft.Y=20;Tft.bold=1;
  Tft.drawString("BlueTooth communication");
  Tft.bold=0;Tft.Y=40;Tft.X=0;
  Tft.drawString("Enablig BT...");
  setBTslave();
  Tft.X=0;
  Tft.drawString("BT is OK, waiting for commands...");
  while(serCommand!=sizeof(str_ser_commands)/ser_command_len){  // loop until comes last defined command: END
    refreshSerInfoTFT();
    waitSerDataCopyToBuff(gBuff);
    //Serial.println(gBuff);
    if (strncmp_P(gBuff, str_ser_asterix,sizeof(str_ser_asterix)-1)==0) {
      // command awaiting, determine what command
      for(byte i=0;i<sizeof(str_ser_commands)/ser_command_len;i++){
        if (strcmp_P(gBuff+2, str_ser_commands[i])==0) {
          serCommand=i+1;
          Tft.X=120;Tft.Y=100;
          Tft.drawString(gBuff+2);
          Tft.drawString("          ");
          break;
        }  
      }
    } else {
      // data is here decide what to do with the data
      switch (serCommand) {
        case 1:  // 
          I2c.stop();  // flush remaining of buffer to the EEPROM
          delay(10);
          serCommand=0;  // reset command because we acted on it already
          break;
        case 2:  // CHIPI2C
          setChipAddrressPageSize(atoi(gBuff));
          serI2CchipPageMask=I2c.chipBuffer-1;
          serCommand=0;  // reset command because we acted on it already
          break;
        case 3:  // ADDRI2C
          serI2Caddr=atoi(gBuff);
          serCommand=0;  // reset command because we acted on it already
          break;
        case 4:  // SNDI2CDAT - Start reading of bytes from serial. Will end if **DONE command received
          serCommand=255;  // switch to another command to receive a byte and sent to the chip
          I2c.setAddress(serI2Caddr);  // initiate a writing to the chip
          I2c.sendByte(atoi(gBuff));
          serI2Caddr++;
          break; 
        case 255:  // receiving a byte and send it to the EEPROM buffer. If end of EEPROM page reached, then initiate page write, and continue...
          if(!(lowByte(serI2Caddr) & serI2CchipPageMask)) {
            // Flush buffer to the EEPROM and initiate new write
            I2c.stop();
            delay(10);
            I2c.setAddress(serI2Caddr);  // initiate a writing to the chip
          } 
          I2c.sendByte(atoi(gBuff));
          serI2Caddr++;
          break;
        case 5:  // SNDI2CU8
          I2c.writeByte(serI2Caddr, atoi(gBuff));
          serI2Caddr++;
          serCommand=0;  // reset command because we acted on it already
          break;
        case 6:  // SNDI2CU16
          I2c.writeWord(serI2Caddr, atoi(gBuff));
          serI2Caddr+=2;
          serCommand=0;  // reset command because we acted on it already
          break;
        case 7:  // GETI2CDAT
          unsigned int bytesToSend = atoi(gBuff);
          // start to request bytes from EEPROM
          I2c.beginRead(serI2Caddr);
          for(bytesToSend;bytesToSend>0;bytesToSend--){
            Serial.println(I2c.readNextByte());
            serI2Caddr++;
            // wait for OK...
            waitSerDataCopyToBuff(gBuff);
            if(strcmp(gBuff,"OK")){break;}
          }
          I2c.finishRead();
          serCommand=0;  // reset command because we acted on it already
          break;
      }
      //Serial.print("Cmd:");Serial.print(serCommand);Serial.print(" ");Serial.println(gBuff);
    }
    Serial.println("OK");
  }
  setChipAddrressPageSize(bitmapsEEPROMchip);  
  setBTmaster();
  clearMainScreen();
}

void waitSerDataCopyToBuff(char* sbuff){
    byte serBuffI=0;
    char recBt=0;
    while(recBt!=13){
      if(Serial.available()){
        recBt=Serial.read();
        if(recBt>31){
          sbuff[serBuffI]=recBt;
          serBuffI++;
        }
      } else { // if nothing comming
          // check for joystick (left) to exit from readSerial routine
          if(readJoystickData()==JOYSTICK_LEFT2){
            //(sizeof(str_ser_commands)/ser_command_len-1)
            strncpy_P(sbuff,str_ser_asterix,2);      // copy ** to the buffer
            strncpy_P(sbuff+2,str_ser_commands[(sizeof(str_ser_commands)/ser_command_len-1)],3);      // copy last command (END) to the buffer after **
            serBuffI=5; // adjust buffer pointer
            break;
          }
      }
      //updateHeaderTFT();
    }
    sbuff[serBuffI]=0;
}

void refreshSerInfoTFT(){
  Tft.Y=60;Tft.X=0;Tft.Fcolor=WHITE;
  Tft.drawString("EEPROM Chip: ");
  Tft.drawNumberR(I2c.chipAddr,5);
  Tft.X=0;Tft.Y=80;
  Tft.drawString("EEPROM addr: ");
  Tft.drawNumberR(serI2Caddr,5);
  Tft.X=0;Tft.Y=100;
  Tft.drawString("Last command:");
}

