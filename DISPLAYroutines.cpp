// All releated to DISPLAY data on TFT

/*typedef struct PGMtext{
  int X;
  byte Y;
  unsigned int Fcolor, Bcolor;
  byte bold;
  char text[32];
};

PGMtext PROGMEM TxtInPGM[]={
  {0,20,YELLOW,BLACK,1, "BlueTooth communication"}
};
*/

void initializeCurrentScreen() {
  if(menuCur==0) {
      clearMainScreen();
      switch (screenCur) {
        case TFTscreens_MainScreen:  // Trip Log
          initializeMainScreenDisplay();          
          break;
        case TFTscreens_Temperature:  // Temperature screen
          initializeTemperatureDisplay();
          break;
        case TFTscreens_SpeedMotor:     // Speed and Motor displaz screen
          initializeSpeedMotorDisplay();
          break;
        case TFTscreens_StartTrip:     // Display Start Trip screen
          initializeStartTripDisplay();
          break;
        case TFTscreens_TripInProgress:     // Display running Trip Log screen
          initializeTripInProgressDisplay();
          break;
        case TFTscreens_ViewTripLog:     // Display Log screen
          initializeTripLogListDisplay();
          break;
      }
  }
//
}

void updateCurrentScreen(){
  if(menuCur==0) {
      switch (screenCur) {
        case TFTscreens_MainScreen:  // main screen
          displayDistance(getOdometerValue());
          break;
        case TFTscreens_Temperature:  // Temperature screen
          if (metroDS18B20Read.check() == 1){
            readTemp();
            displayTemperature();
          }
          break;
        case TFTscreens_SpeedMotor:  // speed and motor data
          drawCurSpeed((byte)CurrentSpeed);
          drawColorBars(MotorLoad);
          break;
        case TFTscreens_TripInProgress:  // update Trip in progress data
          TripInProgressDisplay();
          break;
          
      }
  }
}

void updateOBDiconTFT() {
    if(OBDstatusTFTicon!=obdi.OBDstatus) {
      if(OBDstatusTFTicon!=255) {OBDstatusTFTicon=obdi.OBDstatus;}  // if 255 then will change variable little bit later
      unsigned int tmpAddr=bitmapsEEPROMchip_OBDicon;  // address of OBD icon
      switch (OBDstatusTFTicon) {
        case 1:  //red
          Tft.Fcolor=RED;
          break;
        case 2:  // yellow
          Tft.Fcolor=YELLOW;
        break;
        case 3:  // green
          Tft.Fcolor=GREEN;
        break;
        case 255:  // blue sky BT color
          Tft.Fcolor=BLUE_SKY;
          tmpAddr=bitmapsEEPROMchip_BTexternal;  // address of BT external device bitmap
          OBDstatusTFTicon=obdi.OBDstatus;  // need to change value to prevent repeated drawings of the same picture
        break;
      }
      Tft.Bcolor=0x2945;
      Tft.drawBitmapI2CEEPROM(293, 0, tmpAddr);
      Tft.Bcolor=BLACK;
    }
}

void clearMainScreen() {
   Tft.fillScreen(0,319,17,239, BLACK);
}

void initializeMainScreenDisplay(){
  Tft.Y=60;Tft.X=60;Tft.Fcolor=GRAY1;Tft.drawString("Distance:");
}

void initializeSpeedMotorDisplay(){
  // Draw Engine text
  Tft.Y=220; Tft.X=268;Tft.bold=1;
  Tft.Bcolor=BLACK;Tft.Fcolor=WHITE;
  Tft.drawString("Engine");Tft.bold=0;
  // draw Speed gauge
  byte spdNum=0;
  for(byte i=0; i<20; i+=2){
    Tft.Fcolor=WHITE;
    Tft.X=pgm_read_byte(speedNumCoordTable+i)-10;
    Tft.Y=pgm_read_byte(speedNumCoordTable+i+1)-10;
    //Tft.bold=1;
    Tft.drawNumberI2CEEPROM(spdNum, bitmapsEEPROMchip_10x15Numbers);
    //Tft.drawNumber(spdNum);
    spdNum+=20;
  }
  Tft.bold=0;
  Tft.drawCircle(129+1,177-7, 90);  
}

void initializeTemperatureDisplay(){
    Tft.Fcolor=GRAY1;Tft.Bcolor=BLACK;
    Tft.drawBitmapI2CEEPROM(0, 57, bitmapsEEPROMchip_tempTop); // top
    Tft.drawBitmapI2CEEPROM(149, 91, bitmapsEEPROMchip_tempWindow); // window
    Tft.drawBitmapI2CEEPROM(233, 180, bitmapsEEPROMchip_tempMirror); //mirror
    Tft.drawBitmapI2CEEPROM(0, 210, bitmapsEEPROMchip_tempBottom); // bottom
    displayTemperature();
}

void initializeTripLogListDisplay() {
    listItemsTotal=4;
    listItemOnScreen=1;
    listItemCur=1;
    showItemsList(1);  // type of list, total items, first item on screen, cur item
}


void initializeStartTripDisplay(){
    Tft.Y=50;Tft.X=10;Tft.Fcolor=GREEN;Tft.bold=1;
    Tft.drawString("START Trip >");Tft.bold=0;
    Tft.Y=200;Tft.X=10;
    Tft.drawString("< Exit");
}

void initializeTripInProgressDisplay() {
    // first print static data, then edited values
    //screenElementCur=0;
    printEEPROMstructure(settingsEEPROMchip_printTripProgress);    
    // read last line from Log and show it...
    unsigned int log_addr=getLogLastLineAddr(); // find last address
    setChipAddrressPageSize(logsEEPROMchip);    
    I2c.beginRead(log_addr);
    // read Date
    // char buf[9];
    readEEPROMdate3bytes(gBuff+(globalBuffer_size-10));  // use the end of global buffer (not overwritten by most functions)
    tripStartODO=readEEPROMUlong(); // read start kilometers
    //unsigned long tmpEndKM=readEEPROMUlong();  // do not need to read finish km, because it is not yet defined
    I2c.finishRead();
    setChipAddrressPageSize(bitmapsEEPROMchip);
    // print a date
    printEEPROMstructure(settingsEEPROMchip_printTripProgress1);  //Tft.X=243;Tft.Y=56;Tft.Fcolor=WHITE;
    Tft.drawString(gBuff+(globalBuffer_size-10));
    printEEPROMstructure(settingsEEPROMchip_printTripProgress2);  //Tft.X=90;Tft.Y=227;
    Tft.drawNumber(tripStartODO/1000);    // start km
    screenElementCur=1;
    // initialize Reason and Destination
    tripReasonSelected=1;
    tripDestSelected=1;
}


void displayTemperature(){
  if(menuCur==0) {
      Tft.Y=120;Tft.X=10;Tft.Fcolor=ORANGE;
      Tft.drawFloatI2CEEPROM(temperatureInside,1, bitmapsEEPROMchip_BigNumbers);
      Tft.X=200;Tft.Y=60;Tft.Fcolor=CYAN;
      Tft.drawFloatI2CEEPROM(temperatureOutside,1, bitmapsEEPROMchip_BigNumbers);
  }
}

void displayDistance(unsigned long tDistance) {
  Tft.Y=80;Tft.Fcolor=WHITE;
  Tft.X=60;
  Tft.drawNumberI2CEEPROM(tDistance/1000, bitmapsEEPROMchip_xBigNumbers);
}


void TripInProgressDisplay() {
    setChipAddrressPageSize(settingsEEPROMchip);
    Tft.X=8;Tft.Y=66;Tft.Fcolor=WHITE;Tft.bold=0;  // draw Reason
    drawStringStoredInEEPROM((settingsEEPROMchip_predef_Reasons+1)+(tripReasonSelected-1)*40);
    Tft.fillRectangle(Tft.X, Tft.Y,(0xDF+8)-Tft.X,13);    // clear remaining space
    Tft.X=8;Tft.Y=118;Tft.Fcolor=WHITE;  // draw Destination
    drawStringStoredInEEPROM((settingsEEPROMchip_predef_Dest+1)+(tripDestSelected-1)*40);
    Tft.fillRectangle(Tft.X, Tft.Y,(0xDF+8)-Tft.X,13);    // clear remaining space
    // print distance currently travelled
    unsigned long tmpCurODO=getOdometerValue();
    float curTripDistance=(float)(tmpCurODO-tripStartODO)/1000.0;   // convert to km
    setChipAddrressPageSize(bitmapsEEPROMchip);
    Tft.X=10;Tft.Y=160;Tft.Fcolor=WHITE;Tft.bold=0;
    Tft.drawFloatI2CEEPROM(curTripDistance,1, bitmapsEEPROMchip_BigNumbers);
    Tft.Y=189;Tft.drawString(" km");
    Tft.X=246;Tft.Y=227;
    Tft.drawNumber(tmpCurODO/1000);    
    // Show edited values and Trip command buttons (Finish/Delete)
    printEEPROMstructure(settingsEEPROMchip_printTripProgEdit1);
}
  
// Updating icon and time in the header of TFT
void updateHeaderTFT(){
  if (metroTIMEdisplay.check() == 1){
    DisplayTime();
  }
  updateOBDiconTFT();
  drawWalkingGIF();
}

// color table for bars in PGMSPACE
unsigned int PROGMEM colorBarsTable[]={
  0x0700,  //green
  0x0700,
  0x07E0,
  0x07E0,
  0x87E0,
  0x87E0,
  0xCFE0,
  0xCFE0,
  0xFFE0, //Yellow
  0xFFE0,
  0xFEE0,
  0xFEE0,
  0xFD80,
  0xFD80,
  0xFBC0,
  0xFBC0,
  0xF980,
  0xF980,
  0xF800, //Red
  0xF800
};
//23026
// function for drawing engine load
// OBD giving data from 0 to 255 (0-100%)
void drawColorBars(byte value){
  // TODO use prevEngineLoadValue to implement smooth transitions
  if (prevEngineLoadValue<value) {prevEngineLoadValue++;} else if (prevEngineLoadValue>value) {prevEngineLoadValue--;}
  //value=prevEngineLoadValue;
  value=prevEngineLoadValue*19/255;  //value=map(value, 0, 255, 0, 19);
  byte Ybars=205;
  for(byte i=0;i<20;i++){
    if (value>=i){
      Tft.Bcolor=pgm_read_word(colorBarsTable+i);
    } else {
      Tft.Bcolor=BLACK;
    }
    Tft.fillRectangle(288-i,Ybars,30+i, 5);
    Ybars-=9;
  }
}

void drawCurSpeed(byte value){
  Tft.X=133;
  Tft.Fcolor=BLUE_SKY;Tft.Y=140;  
  drawNumberI2CEEPROMcenter(value,3);
  //249,277
}

// Coordinates given by Tft X and Y. and pointing to the center. Returns nothing
void drawNumberI2CEEPROMcenter(long val, byte Syms){
  #define charWidth (33+(33/4))
  #define charHeight 54
  // calculate left most pos
  int leftX=Tft.X-charWidth*Syms/2;
  // calculate nums start coordinate
  byte i;
  long powN=1;
  for(i=1;i<9;i++){ // count digits in long number
    powN*=10;
    if(val<powN){break;}
  }
  byte tmpBoxW=0;
  if(i<Syms){  //draw two filled boxes to clear prev possible numbers on TFT
    tmpBoxW=(Syms-i)*charWidth/2;  // one box width
    Tft.fillRectangle(leftX, Tft.Y, tmpBoxW,charHeight);
    Tft.fillRectangle(leftX+tmpBoxW+charWidth*i, Tft.Y, tmpBoxW,charHeight);
  }
  Tft.X=leftX+tmpBoxW+1;  //
  Tft.drawNumberI2CEEPROM(val,bitmapsEEPROMchip_xBigNumbers);
}


// Draw a walking man indicating that trip Log in progress
// uses a global byte variable:
// 0 - no bitmap showed, >0 - number of bitmap to show
// another global unsigned long is for millis timing.
void drawWalkingGIF(){
    Tft.Fcolor=WHITE;Tft.Bcolor=0x2945;
    if(walkingCounter==0) {  // do we need to draw a GIF?
        if(walkingMillis!=0) {  // millis variable not cleared, so clear GIF
            // draw a empty bitmap
            Tft.drawBitmapI2CEEPROM(270, 0, bitmapsEEPROMchip_WalkingGIF); // 0 offset is empty bitmap
            walkingMillis=0;    // do not clear empty area on every function call
        }
    } else {    // walking is activated, lets compare millis
        if(millis()-walkingMillis>walking_Speed) {
            // show next bitmap
            walkingMillis=millis();
            Tft.drawBitmapI2CEEPROM(270, 0, bitmapsEEPROMchip_WalkingGIF+(walkingCounter)*walking_bitmapSize); // 0 offset is empty bitmap
            walkingCounter++;
            if(walkingCounter>5){walkingCounter=1;}
        }
    }
    Tft.Bcolor=BLACK;
}
