// routines for Trip Log

// writes starting trip data to the EEPROM
void startTrip() {
    // update settings in EEPROM
    settingsWriteByte(settingsEEPROMchip_TripStarted, 1);
    // Add new line to the Log EEPROM
    unsigned int log_addr=getLogLastLineAddr(); // find last address
    // get odometer value
    unsigned long curOdo=getOdometerValue();    // gets data from Settings EEPROM chip
    //I2c.stop();
    // start write:
    DateTime now = RTC.now();  // must be before direct EEPROM operations (uses other address for RTC)
    setChipAddrressPageSize(logsEEPROMchip);    
    I2c.setAddress(log_addr);
    // write a date (3 bytes)
    I2c.sendByte(now.day());
    I2c.sendByte(now.month());
    I2c.sendByte(now.year()-2000);
    // write start km
    writeEEPROMUlong(curOdo);
    // do not need to write end kilomeeters right now
    // write empty end kilomeeters, Destination and reason (just 84 bytes)
    for(byte i=0;i<logsEEPROMchip_LastByte-logsEEPROMchip_LineFinish;i++){I2c.sendByte(0);}
    I2c.stop();
    delay(10);
    // at the end restore the chip address
    setChipAddrressPageSize(bitmapsEEPROMchip);
    //screenElementCur=1;   // this variable used here for selected element on Trip screen
    walkingCounter=1;  // Strat walking GIF
}

// write finish trip data to the EEPROM
void finishTrip() {
    settingsWriteByte(settingsEEPROMchip_TripStarted, 0);   // finish a trip
    // find last address
    // Increment Loglines counter
    unsigned int log_lines=settingsReadWord(settingsEEPROMchip_TotalLinesInLog);
    unsigned int log_addr=log_lines*logsEEPROMchip_sizeLine;
    log_lines++;
    settingsWriteWord(settingsEEPROMchip_TotalLinesInLog, log_lines);
    unsigned long curOdo=getOdometerValue();    // gets data from Settings EEPROM chip
    // read selected Destination and Reason from EEPROM to global buffer
    setChipAddrressPageSize(settingsEEPROMchip);
    I2c.readBuffer((settingsEEPROMchip_predef_Dest+1)+(tripDestSelected-1)*40, gBuff, 40);
    I2c.readBuffer((settingsEEPROMchip_predef_Reasons+1)+(tripReasonSelected-1)*40, gBuff+40, 40);
    // change to Log EEPROM and write...
    setChipAddrressPageSize(logsEEPROMchip);    
    I2c.setAddress(log_addr+logsEEPROMchip_LineFinish);     // jump to end km address
    writeEEPROMUlong(curOdo);   // write odometer cur value as end km
    for(byte i=0;i<80;i++){I2c.sendByte(gBuff[i]);} // write Destination and Reason
    I2c.stop();
    delay(10);  // wait for EEPROM Write
    setChipAddrressPageSize(bitmapsEEPROMchip);    
    walkingCounter=0;   // stop walking GIF
}

void cancelTrip() {
    // update settings in EEPROM
    settingsWriteByte(settingsEEPROMchip_TripStarted, 0);
    unsigned int log_lines=settingsReadWord(settingsEEPROMchip_TotalLinesInLog);
    log_lines--;
    settingsWriteWord(settingsEEPROMchip_TotalLinesInLog, log_lines);
    //menuCurSelLine=0;   // this variable used here for selected element on Trip screen
    walkingCounter=0;  // Strat walking GIF
}

//exit from Trip screens to Trip submenu
void exitTripScreen() {
    screenCur=screenCurForRestore;  // after exit main menu go to screen before entering to the Trip screens
    setMenu(&menu1[0], 3); // set Log subMenu
}


// Show a list of items on TFT
#define itemsPerScreen 10   // how many items list at a time on TFT
#define itemsXpos 20    // position on screen where to print list
#define itemsYpos 20    // position on screen where to print list
#define itemsSpacing 20 // distance between top of the lines
void showItemsList(byte itemsType){
    setChipAddrressPageSize(logsEEPROMchip);
    byte Ypos=itemsYpos;
    int tmpCurPLine=listItemCur-(listItemOnScreen-1);    // Line in List from we start to fill TFT screen
    for(byte i=0;i<itemsPerScreen;i++){
        if(listItemsTotal<tmpCurPLine){break;}
        switch (itemsType) {
        case 1:
            listPrintCurLineLog(tmpCurPLine, itemsXpos, Ypos);
            break;
        }
        tmpCurPLine++;
        Ypos+=itemsSpacing;
    }
    // list is printed, mark current item
    //if(listItemCur-listItemTop<itemsPerScreen){
    setChipAddrressPageSize(bitmapsEEPROMchip); // let's restore default chip
    Tft.fillRectangle(0, itemsYpos, itemsXpos, itemsPerScreen*itemsSpacing); // fill with Bcolor
    Tft.Fcolor=WHITE; Tft.drawBitmapI2CEEPROM(0, itemsYpos+(listItemOnScreen-1)*itemsSpacing, bitmapsEEPROMchip_CurItemList);  // 16, 13
    //}
}

void goNextItemList() {
    if(listItemCur<listItemsTotal){  // can we go next at all?
        listItemCur++;
        if(listItemOnScreen<itemsPerScreen) {      // do we need to change cursor position also (if cursor at the end)?
            listItemOnScreen++;
        }
    }
}

void goPrevItemList() {
    if(listItemCur>1){  // can we go prev at all?
        listItemCur--;
        if(listItemOnScreen>1) {      // do we need to change cursor position also (if cursor at the end)?
            listItemOnScreen--;
        }
    }
}


// lines enumerated for 1
void listPrintCurLineLog(int CurPLine, int Xpos, byte Ypos){
    // find a data for Line in EEPROM
    unsigned int curLineAddr=(CurPLine-1)*logsEEPROMchip_sizeLine+logsEEPROMchip_LineDate;
    // start to read the data
    I2c.beginRead(curLineAddr);  // start to read a log line
    // read and print a date
    //char buf[9];
    readEEPROMdate3bytes(gBuff);  // Global buffer is used
    Tft.drawString(gBuff);
    // read and print kilometers
    unsigned long tmpStrtKM=readEEPROMUlong();
    unsigned long tmpEndKM=readEEPROMUlong();
    I2c.finishRead();
    Tft.drawNumberR(tmpEndKM-tmpStrtKM,5);
    Tft.drawString(" km - ");
    // read and print the reason
    drawStringStoredInEEPROM(curLineAddr+logsEEPROMchip_LineReason);   // Print Reason
    // clear remaining space in line
    Tft.fillRectangle(Tft.X, Tft.Y, 318-Tft.X, itemsSpacing); // fill with Bcolor
}

// this is not a standalone function
// EEprom address must be set and reading is started with I2c.beginRead(...)
// Date is stored in EEPROM in 3 bytes (day,month,year)
// date will be placed to 9 bytes buffer terminated with 0.
void readEEPROMdate3bytes(char* buf) {
    //byte bufP;    // pointer in buffer
    for(byte i=0;i<8;i+=3){
        byte bufP=i; // pointer to the dot 
        byte tmpB=I2c.readNextByte();
        if (tmpB >= 0 && tmpB < 10) {buf[bufP]='0';bufP++;}
        itoa(tmpB, buf+bufP,10);    // number to string
        buf[i+2]='/';
    }
    buf[8]=0;//replace last dot with 0
}
