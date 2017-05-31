char PROGMEM monthNames[12][4] = {"Jan","Feb","Mar","Apr","Mai","Jun","Jul","Aug","Sep","Oct","Nov","Dec"}; // Months names

unsigned long adjustDriftRTC(DateTime curdt) {
  //correctRTCevery
  unsigned long curtime=curdt.unixtime();
  unsigned long lastadjtime=RTCreadDriftDate();
  if (curtime>=lastadjtime+correctRTCevery) {
    //Serial.print("Time before adjusting: ");Serial.println(curtime);
    // determine adjusting amount
    unsigned long diff=correctRTCvalue*(curtime-lastadjtime)/correctRTCevery;
    curtime+=diff;
    RTC.adjust(DateTime(curtime));
    RTCwriteDriffDate();
    //Serial.print("Time is adjusted for: ");Serial.print(diff);Serial.println(" seconds");
    return 0;
  }
  return lastadjtime+correctRTCevery-curtime;
}

void RTCwriteDriffDate() {
  // read from RTC and write to RAM of RTC
  unsigned long utime = RTC.now().unixtime();
  I2c.start();
  I2c.sendByte(SLA_W(0x68));
  I2c.sendByte(8); // user RAM space
  writeEEPROMUlong(utime);
  //for (byte i=0; i<25; i+=8){I2c.sendByte((byte)(utime >> i));}    // write long to 4 bytes of RTC user RAM
  I2c.stop();
}

unsigned long RTCreadDriftDate() {
  unsigned long utime=0;
  I2c.start();
  I2c.sendByte(SLA_W(0x68));
  I2c.sendByte(8);  // RAM start address
  I2c.start();
  I2c.sendByte(SLA_R(0x68));
  for (byte i=0; i<25; i+=8){
    I2c.receiveByte(i!=24);     // last byte will be sent with NOACK (0)
    utime+=(unsigned long)TWDR << i;
  }
  I2c.stop();  
  return utime;
}

void DisplayTime() {
    DateTime now = RTC.now();
    long toRTCupdate= adjustDriftRTC(now);
    Tft.bold=0;
    Tft.Y=3;Tft.X=2;Tft.Fcolor=WHITE;Tft.Bcolor=0x2945;
    //char buff[4];
    strcpy_P(gBuff, monthNames[now.month()-1]); // global buffer used
    Tft.drawString(gBuff);
    //Tft.drawChar(' ');
    Tft.drawNumberR(now.day(),3);
    Tft.drawString("__"); // big spaces
    Tft.X=130;Tft.Fcolor=GREEN;
    Tft.drawNumber(now.hour());
    Tft.drawChar(':'); 
    print2digits(now.minute());
    Tft.drawChar(':'); 
    print2digits(now.second());
    Tft.Bcolor=BLACK;
}


int print2digits(byte number) {
//  int curpos=0;
  if (number >= 0 && number < 10) {Tft.drawChar('0');}
  Tft.drawNumber(number);
}
