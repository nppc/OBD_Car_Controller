PROGMEM prog_uchar addr1[8]= {0x28, 0x48, 0xF6, 0x4C, 0x05, 0x00, 0x00, 0x37};  //DS18B20
PROGMEM prog_uchar addr2[8]= {0x28, 0x82, 0xDF, 0x45, 0x05, 0x00, 0x00, 0xFF};  //DS18B20


//returns the temperature from one DS18S20 in DEG Celsius
void readTemp(){
  _readTemp(addr1, temperatureInside);
  _readTemp(addr2, temperatureOutside);
}


float _readTemp(prog_uchar *addrP, float &answer)
{
  byte addr[8];
  memcpy_P(addr, addrP, 8);
  // read data from last conversion (at first call after power on data will be wrong) 
  ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad
  byte LSB =  ds.read();
  byte MSB =  ds.read();

  float tempRead = ((MSB << 8) | LSB); // using two's compliment
  answer = tempRead / 16;
  // start next conversion
  ds.reset();
  ds.select(addr);
  ds.write(0x44); // start conversion, with external power on at the end
  ds.reset();
}
