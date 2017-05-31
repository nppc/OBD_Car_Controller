/* My menu structure in FLASH
Every element of the menu can be addressed by two bytes:
  number of block, number of line in the menu
DATA for every block:
2 byte - quantity of elements in this menu
------menu items-------
2 byte - address of the menu if go right. if 0xFFFF then exit from menu
2 byte - address of the menu if go left. if 0xFFFF then exit from menu
1 byte - if not 0 then run Inc routine
1 byte - if not 0 then run Dec routine
20 bytes - text of the menu
*/

// for global vars see definitions.h

void doMenuLeft(byte &cMenu, byte &cMline) {
  if(cMenu!=0){
    unsigned int addr=menuAddr+sizeof(mnuHdr)*(cMenu-1);
    unsigned int menuLaddr=pgm_read_word(addr+2)+sizeof(mnuLines)* (cMline-1);
    // get 'left' byte for cur line
     byte actByte= pgm_read_byte(menuLaddr);
     //Serial.print("actByte: ");Serial.println(actByte);
     clearMainScreen();
     if(actByte!=0xFF){
       cMenu=actByte;
       cMline=1;
     } else {
       if(cMenu==1){
           cMenu=0;
           // hide menu (exit from menu)
           menuCurSelLine=0; // reset this variable to reuse it  
           initializeCurrentScreen();
           //Serial.println("Exit from menu");
       }
     }
  }
}

void doMenuRight(byte &cMenu, byte &cMline) {
  if(cMenu!=0){
    unsigned int addr=menuAddr+sizeof(mnuHdr)*(cMenu-1);
    unsigned int menuLaddr=pgm_read_word(addr+2)+sizeof(mnuLines)* (cMline-1);
    byte actByte=pgm_read_byte(menuLaddr+1);
    clearMainScreen();
    if(actByte!=0){
      cMenu=actByte;
      cMline=1;
    } else {
      // act on selected menu
       doMenuSelectedItem(cMenu, cMline);
       //Serial.println("Do selected menu");
       cMenu=0;
       // hide menu (exit from menu)
       initializeCurrentScreen();
    }
  }
}

void showMenu(byte cMenu, byte &cMline) {
  if(cMenu!=0){
    Tft.Y=25;
    //static char str_buffer[lengthMenuLineStr]; // use Global buffer
    // search for the menu
    unsigned int addr=menuAddr+sizeof(mnuHdr)*(cMenu-1);
    byte menuLines=pgm_read_byte(addr+1);
    if(cMline>menuLines){cMline=1;}  // if Cur menu line is out of bounds then cycle 
    if(cMline<1){cMline=menuLines;}  // if Cur menu line is out of bounds then cycle 
    // read and show menu lines
    unsigned int menuLaddr=pgm_read_word(addr+2);
    for(byte i=0; i<menuLines;i++){
      // Print a menu line
      Tft.bold=1;Tft.Bcolor=BLACK;Tft.X=10;Tft.Fcolor=BLUE_SKY;
      Tft.drawRectangle(Tft.X,Tft.Y,300,30);
      if (cMline==i+1) {// mark a current line
        Tft.Bcolor=BLUE;
        //Serial.print("* ");
      }
      // fill all around text
      int x5var=Tft.X+5; byte y1var=Tft.Y+1;  // Y will be 239 max
      Tft.fillRectangle(Tft.X+1, y1var,3,28);
      Tft.fillRectangle(x5var, y1var,294,6);
      Tft.fillRectangle(x5var, Tft.Y+21,294,8);
      Tft.X+=5;Tft.Y+=8;Tft.Fcolor=WHITE;Tft.drawString(strcpy_P(gBuff, (char*)(menuLaddr+(sizeof(mnuLines)-lengthMenuLineStr))));
      Tft.fillRectangle(Tft.X, Tft.Y,309-Tft.X,12);
      //Serial.println(strcpy_P(gBuff, (char*)(menuLaddr+(sizeof(mnuLines)-lengthMenuLineStr))));
      menuLaddr+=sizeof(mnuLines); // 24 size of mnuLines structure
      Tft.Y+=37;
    }
    //Serial.println();
    Tft.bold=0;
  }
}


// Routine for initializing and show a menu.
// mnuHdr - address of headers of menu.
// selSubMenu - Sub menu to show
void setMenu(struct mnuHdr *adrMnu, byte selSubMenu){
  menuAddr=(unsigned int)adrMnu; 
  //menuCur=1;
  menuCur=selSubMenu;
  menuCurSelLine=1;
  clearMainScreen();
  showMenu(menuCur, menuCurSelLine);  
}

void doMenuSelectedItem(byte &cMenu, byte &cMline){
  //menuCurSelLine=0; // reset this variable to reuse it 
  unsigned int menuCmd=cMenu*256+cMline;
  switch (menuCmd) {
    case 0x0101:  // Main screen
      screenCur=TFTscreens_MainScreen;
      break;
    case 0x0103:  // temperature
      screenCur=TFTscreens_Temperature;
      break;
    case 0x0104:  // Speed and motor data
      screenCur=TFTscreens_SpeedMotor;
      break;
    case 0x0201:  // read from serial and store to I2C
      readSerial();
      break;
    case 0x0301:  // Start or End Trip
      screenCurForRestore=screenCur;
      if(settingsReadByte(settingsEEPROMchip_TripStarted)==0){screenCur=TFTscreens_StartTrip;} else {screenCur=TFTscreens_TripInProgress;}
      break;
    case 0x0302:  // View Log Data
      screenCur=TFTscreens_ViewTripLog;
      break;
  }
}

