
// This table and function sets I2C EEPROM chip address, and determines a internal chip buffer and page siye for page write operations
// If you do not need to set a page size, the just call I2c.setChip(c_addr);, it is little bit faster
const byte PROGMEM I2CChipBufferSizes[][2] = {  // Table of I2C chip addresses and buffer len (mask). (if read by word then low byte/high byte)
  {0x50, 32},      // I2C ATMEL chip with 4Kb EEPROM (32 bytes buffer)
  {0x51, 64},      // I2C ATMEL chip with 16Kb EEPROM (64 bytes buffer)
  {0x52, 128}      // I2C MICROCHIP chip with 64Kb EEPROM (128 bytes buffer)
};

void setChipAddrressPageSize(byte c_addr) {
  I2c.chipAddr=c_addr;
  for(byte i=0;i<sizeof(I2CChipBufferSizes)/2;i++){
    unsigned int tmpVal=pgm_read_word(I2CChipBufferSizes[i]);
    if(lowByte(tmpVal)==c_addr){
      I2c.chipBuffer=highByte(tmpVal);
    }
  }
}


// Function reads and prints a line (terminated by zero) from EEPROM starting at addr.
// It uses current chip address
void drawStringStoredInEEPROM(unsigned int addr) {
   I2c.beginRead(addr); 
   char curC = I2c.readNextByte(); // read the first byte
   while (curC!=0) {
    Tft.drawChar(curC);
    curC= I2c.readNextByte();
   }
   I2c.finishRead();
}

// this is a not standalone function
// it requires that I2c.beginRead(curLineAddr); at the beginning
// and at the end I2c.finishRead();
unsigned long readEEPROMUlong() {
    unsigned long value=0;
    for (byte i=0; i<25; i+=8){
        value +=(unsigned long)I2c.readNextByte() << i;
    }
    return value;
}


// this is a not standalone function
// it requires that I2c.beginRead(curLineAddr); at the beginning
// and at the end I2c.finishRead();
unsigned int readEEPROMUint() {
    unsigned int value=0;
    value=I2c.readNextByte()*256;
    value+=I2c.readNextByte();    
    return value;
}


// this is a not standalone function
// it requires that I2c.setAddress(curLineAddr); at the beginning
// and at the end I2c.stop();
void writeEEPROMUlong(unsigned long value) {
    for (byte i=0; i<25; i+=8){I2c.sendByte((byte)(value >> i));}    // write long to 4 bytes of EEPROM
}


byte settingsReadByte(unsigned int s_addr) {
    setChipAddrressPageSize(settingsEEPROMchip);
    byte val=I2c.readByte(s_addr);
    setChipAddrressPageSize(bitmapsEEPROMchip);
    return val;
}

void settingsWriteByte(unsigned int s_addr, byte data) {
    setChipAddrressPageSize(settingsEEPROMchip);
    I2c.writeByte(s_addr, data);
    setChipAddrressPageSize(bitmapsEEPROMchip);
    delay(10);
}

unsigned int settingsReadWord(unsigned int s_addr) {
    setChipAddrressPageSize(settingsEEPROMchip);
    unsigned int val=I2c.readWord(s_addr);
    setChipAddrressPageSize(bitmapsEEPROMchip);
    return val;
}

void settingsWriteWord(unsigned int s_addr, unsigned int data) {
    setChipAddrressPageSize(settingsEEPROMchip);
    I2c.writeWord(s_addr, data);
    setChipAddrressPageSize(bitmapsEEPROMchip);
    delay(10);
}

// Stores current Odometer value from memorz to the eeprom if:
//   current speed is 0 and traveled more than 10 meeters from last save, or
//   if traveled more than 5 kilomeeters (from last save) and speed is not zero.
// The variable TraveledDistance contains a distance traveled since last save (well be zeroed after save).
// ATMEL I2C EEPROM have 1 million wryte cycle, so it is enough for this algorhytm
void storeOdometerValueEEPROM() {
    //Tft.Bcolor=BLACK;Tft.fillRectangle(90,2,10, 12);
    if((CurrentSpeed==0 && TraveledDistance>5.0) || (CurrentSpeed!=0 && TraveledDistance>5000.0)) {
        // write to EEPROM (read last value, and accumulate...)
        setChipAddrressPageSize(settingsEEPROMchip);
        I2c.beginRead(settingsEEPROMchip_OdometerValue); 
        unsigned long lastOdo=readEEPROMUlong();
        I2c.finishRead();
        unsigned int dTraveled=(unsigned int)TraveledDistance;  // not need to use long variable, because dTraveled must not go much bigger than 5000
        lastOdo+=dTraveled;
        // store back to the EEPROM
        //Tft.Bcolor=YELLOW;Tft.fillRectangle(90,2,10, 12);Tft.Bcolor=BLACK;
        I2c.setAddress(settingsEEPROMchip_OdometerValue);
        writeEEPROMUlong(lastOdo);
        I2c.stop();
        delay(10);
        setChipAddrressPageSize(bitmapsEEPROMchip);
        // zero a TraveledDistance variable, but left float part (for accuracy)
        TraveledDistance-=(float)dTraveled; // if TraveledDistance had something after . then it will stay in TraveledDistance.
    } 
}

// reads Odometer value from EEPROM and adds value from TraveledDistance
unsigned long getOdometerValue() {
    setChipAddrressPageSize(settingsEEPROMchip);
    I2c.beginRead(settingsEEPROMchip_OdometerValue); 
    unsigned long lastOdo=readEEPROMUlong();
    I2c.finishRead();
    setChipAddrressPageSize(bitmapsEEPROMchip);
    return lastOdo+(unsigned int)TraveledDistance;
    //return 25;
}

unsigned int getLogLastLineAddr() {
    unsigned int log_lines=settingsReadWord(settingsEEPROMchip_TotalLinesInLog);
    return log_lines*logsEEPROMchip_sizeLine;
}

// print structure from Settings EEPROM to the TFT (useful to print data to exact place on TFT)
// variable menuCurSelLine must be zero if only text will be printed, but >0 if need to select certain element
void printEEPROMstructure(unsigned int addr) {
    byte totI=settingsReadByte(addr);   // read number of lines in a structure
    byte isCursor = totI & 0x80;
    totI=totI & 0x7F;   // clear cursor bit
    addr++;
    if(screenElementCur>totI) {screenElementCur=totI;}    // if selected item out of bounds then adjust it
    for (byte i=1;i<=totI;i++) {
        setChipAddrressPageSize(settingsEEPROMchip);
        I2c.beginRead(addr); 
        byte lType=I2c.readNextByte();  // read type of the Line
        Tft.X=readEEPROMUint(); // set TFT X coord.
        Tft.Y=readEEPROMUint(); // set TFT Y coord.
        unsigned int lWidth=readEEPROMUint();   // width
        unsigned int lHeight=readEEPROMUint();   // Height
        // enought data for draw or clear a box around text selecting it (if needed)
        if(isCursor) {  // if needed
            if (screenElementCur==i) {Tft.Fcolor=YELLOW;} else {Tft.Fcolor=BLACK;}    // set or clear
            Tft.drawRectangle(Tft.X-4, Tft.Y-4, lWidth+8, lHeight+8);   // draw around a defined text
        }
        Tft.Fcolor=readEEPROMUint(); // set TFT Fcolor.
        Tft.Bcolor=readEEPROMUint(); // set TFT Bcolor.
        Tft.bold=I2c.readNextByte(); // set TFT bold
        for(byte l=0;l<21;l++){gBuff[l]=I2c.readNextByte();if(l>1 && gBuff[l]==0){break;}} // read 21 bytes of data or less (min 2 bytes) to RAM buffer.
        I2c.finishRead();
        addr+=printEEPROMstructure_lineSize;
        // 21 bytes of data or less, already read to RAM global buffer. From now we can use another EEPROM chip if needed
        // decide how to use data from buffer
        switch (lType) {
        case 1:  // print a string from global buffer
          Tft.drawString(gBuff);
          break;
        case 2:  // read a string from another settings EEPROM location (if text bigger than 20 symbols)
          drawStringStoredInEEPROM(gBuff[0]*256+gBuff[1]);    // read address from buffer
          break;
        case 3:  // Draw a rectangle
          Tft.drawRectangle(Tft.X, Tft.Y, lWidth, lHeight);
          break;
        case 4:  // fill a rectangle
          Tft.fillRectangle(Tft.X, Tft.Y, lWidth, lHeight);
          break;
        }
    }
    setChipAddrressPageSize(bitmapsEEPROMchip);   
}
