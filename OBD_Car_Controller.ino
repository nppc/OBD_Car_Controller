/******************************************************
* Known problemen:                                    *
* + Shows speed higher than normal                    *
* + Updates Speed and motor load once per 1 or 2 sec  *
* + Updates distance once in 40-50 seconds            *
* + OBD connection icon not changes color from green  *
*******************************************************/
#include <Arduino.h>
#include <avr/pgmspace.h>
//#include <SoftwareSerial.h>
#include <Metro.h>
#include <SPI.h>
#include <TFTv2.h>
#include <I2C.h>
#include <OneWire.h>
#include "RTClibFastI2c.h"
#include "definitions.h"
#include <OBDINT.h>


//SoftwareSerial softSerial(3, 2); // RX, TX

RTC_DS1307 RTC;
OneWire ds(A3); // on digital pin A3

OBDI obdi();  // start OBD initialization process

//Metro metroOBDSRead = Metro(25);     // 0.025sec
Metro metroTIMEdisplay = Metro(500); // 0.5sec
Metro metroDS18B20Read = Metro(5000); // 5sec

float temperatureInside;
float temperatureOutside;

int SpeedReadCounter = 0;	// counter for acumulating speed readings for average value
int SpeedReadAcumVal = 0;	// acumulates speed readings for  average value
float SpeedAverageInGap = 0; // calculated (averaged) speed for using in other (trip) calculations
unsigned long SpeedMeasureStartTime;	// Start time of measuring a speed for a gap.
unsigned long SpeedMeasureInterval;	// Start time of measuring a speed for a gap.
byte OBDstatusTFTicon=0;  // last status drawed on TFT. 0 is not existing value, so icon will be updated on first try

void setup()
{
       pinMode(BT_PIO11Pin, OUTPUT);
       digitalWrite(BT_PIO11Pin, LOW);  // BT in COM mode
       pinMode(BT_ResetPin, OUTPUT);
       digitalWrite(BT_ResetPin, LOW);  // Leave BT in RESET

  	Serial.begin(9600);
        I2c.begin();
        //Wire.begin();
        setChipAddrressPageSize(bitmapsEEPROMchip);
        RTC.begin();

        readTemp();  // initialize temperature readings (must call not often than once per second)
        
	Tft.TFTinit();  // init TFT library
        Tft.fillScreen(WHITE);
        Tft.Fcolor=0x44EA;Tft.Bcolor=WHITE;
        Tft.drawBitmapI2CEEPROM(60, 20, bitmapsEEPROMchip_mazdaLogo); // Mazda logo
         
         setBTmaster();    // Reset BT device - ensure that in COM mode

        Tft.fillScreen(0,319,0,16, 0x2945);
        clearMainScreen();
        Tft.Bcolor=BLACK;
        
// scroll
/*
    Tft.sendCMD(0x33);      // Set Scroll area
    Tft.WRITE_DATA(0x00);  // TFA
    Tft.WRITE_DATA(0x00);
    Tft.WRITE_DATA(highByte(320-100));  // VSA
    Tft.WRITE_DATA(lowByte(320-100));
    Tft.WRITE_DATA(0x00);  // BFA
    Tft.WRITE_DATA(100);

for(int y=1; y<10;y++) {
  for(int i=1; i<320; i++) {
    Tft.sendCMD(0x37);      // Verticall scrolling start address
    Tft.WRITE_DATA(highByte(i));
    Tft.WRITE_DATA(lowByte(i));
//    delay(10);
  }

}
*/
// end scroll

  screenCur=TFTscreens_SpeedMotor;
  initializeCurrentScreen();
  walkingCounter=settingsReadByte(settingsEEPROMchip_TripStarted);    // start walking GIF if needed

}

void loop()
{
    if (OBDstatusTFTicon!=255) {
      strcpy_P(gBuff,OBDdataCMD[curOBDcmd]);    // Global buffer
      if(obdi.sendCommand(gBuff)) {
        if (SpeedReadCounter==0) {SpeedMeasureStartTime= millis();}	// initialize for speed measurement.
        curOBDcmd++;  // go to next command
        if(curOBDcmd==sizeof(OBDdataCMD)/5){curOBDcmd=0;}  // reset counter when all commands are read from OBDdataCMD	
      }
    }
  if(OBDstatusTFTicon!=255) {  // only proccess OBD routines, if BT is not slave
    if (obdi.checkOBD()==OBDReader_DATA) {
  	  if(strncmp(obdi.readBuffer(), "410D", 4)==0) {
        //Tft.Bcolor=GREEN;Tft.fillRectangle(75,2,5, 12);Tft.Bcolor=BLACK; // debug
        obdi.OBDstatus=3;  // green icon
        byte curSpeed=hex2uint8(obdi.readBuffer(4));
        CurrentSpeed=curSpeed; //* (float)SpeedCorrCoef;  // can display current speed on TFT
        SpeedReadAcumVal+=curSpeed;
        SpeedReadCounter++;
  	    if (SpeedReadCounter==TimeGap_AvgSpeedReadCnt) {
  	      SpeedAverageInGap = (float)SpeedReadAcumVal / (float)TimeGap_AvgSpeedReadCnt;
  	      SpeedReadCounter = 0;
  	      SpeedReadAcumVal = 0;
  		// Calculate trip distance for this gap
  		// (avg km/h) * 1000    meeters in 1 hour (3600sec)
  		// (avg m/h) * 5sec / 3600sec     get distance in 5 sec gap (3600 secs in 1 hour)
              // where is 5sec time while all measurements for given period are done.
  	      TraveledDistance += SpeedAverageInGap * (float)DistanceCorrCoef * ((float)millis()-SpeedMeasureStartTime) / 3600.0; // accumulate distance (in meters)
              //displayCurSpeedAndDistance(CurrentSpeed, TraveledDistance);
  	    }
  	  }
          if(strncmp(obdi.readBuffer(), "4104", 4)==0) {
            //Tft.Bcolor=BLUE;Tft.fillRectangle(75,2,5, 12);Tft.Bcolor=BLACK; // debug
            obdi.OBDstatus=3;  // green icon
            MotorLoad=hex2uint8(obdi.readBuffer(4));  // 0 - 255
          }
    }
  }  

  
  updateHeaderTFT();
  
  updateCurrentScreen();
  
  processJoystick();
  
  storeOdometerValueEEPROM();

  //Tft.Bcolor=BLACK;Tft.fillRectangle(70,2,10, 12);  // debug

// END of LOOP
}

byte hex2uint8(const char *p)
{
	unsigned char c1 = *p;
	unsigned char c2 = *(p + 1);
	if (c1 >= 'A' && c1 <= 'F')
		c1 -= 7;
	else if (c1 >='a' && c1 <= 'f')
	    c1 -= 39;
	else if (c1 < '0' || c1 > '9')
		return 0;

	if (c2 >= 'A' && c2 <= 'F')
		c2 -= 7;
	else if (c2 >= 'a' && c2 <= 'f')
	    c2 -= 39;
	else if (c2 < '0' || c2 > '9')
		return 0;

	return c1 << 4 | (c2 & 0xf);
}

//26542
