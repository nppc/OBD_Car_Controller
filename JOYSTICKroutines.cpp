
//int portXval; int portYval;

byte readJoystickData() {
  int portXval=analogRead(JOYSTICK_PINX);
  int portYval=analogRead(JOYSTICK_PINY); 
  byte outval=0;
  if(portXval>JOYSTICK_RIGHTmax) {
    outval+=JOYSTICK_RIGHT2;
  }
  if(portXval<JOYSTICK_LEFTmax) {
    outval+=JOYSTICK_LEFT2;
  }
  if(portYval>JOYSTICK_UPmax) {
    outval+=JOYSTICK_UP2;
  }
  if(portYval<JOYSTICK_DOWNmax) {
    outval+=JOYSTICK_DOWN2;
  }
  return outval;
}

// last reading of buttons state. Need to remeber it to avoid multiple screen initializaiton (while button is pressed)
byte curBUTTONSstate=0;  // value from buttons reading function

void processJoystick() {
  
  byte newBUTTONtmp=readJoystickData();
  if(newBUTTONtmp!=curBUTTONSstate){
    // button is changed own state
    curBUTTONSstate=newBUTTONtmp;
    if (menuCur>0) {  // Menu is activated, let's process menu movements
      switch (curBUTTONSstate) {
      case JOYSTICK_UP2:
        menuCurSelLine--;
        break;
      case JOYSTICK_DOWN2:
        menuCurSelLine++;
        break;
      case JOYSTICK_LEFT2:
        doMenuLeft(menuCur,menuCurSelLine);
        break;
      case JOYSTICK_RIGHT2:
        doMenuRight(menuCur,menuCurSelLine);
        break;
      }
      showMenu(menuCur,menuCurSelLine);
    } else {  // Process normal joystick operations in Screens
        switch (screenCur) {
        case TFTscreens_Temperature:
        case TFTscreens_SpeedMotor:
        case TFTscreens_MainScreen:
          switch (curBUTTONSstate) {
          case JOYSTICK_UP2:
            screenCur=TFTscreens_MainScreen;
            initializeCurrentScreen();
            break;
          case JOYSTICK_DOWN2:
            screenCur=TFTscreens_SpeedMotor;
            initializeCurrentScreen();
            break;
          case JOYSTICK_LEFT2:
            setMenu(&menu1[0], 1);  // set Main Menu
            break;
          case JOYSTICK_RIGHT2:
            screenCur=TFTscreens_Temperature;
            initializeCurrentScreen();
            break;
          }
          break;
        case TFTscreens_ViewTripLog:
          switch (curBUTTONSstate) {
          case JOYSTICK_UP2:
            // Move pointer to the previous line of Trip Log
            goPrevItemList();
            showItemsList(1);  // type of list, total items, first item on screen, cur item
            break;
          case JOYSTICK_DOWN2:
            // Move pointer to the next line of Trip Log
            goNextItemList();
            showItemsList(1);  // type of list, total items, first item on screen, cur item
            break;
          case JOYSTICK_LEFT2:
            setMenu(&menu1[0], 3); // set Log subMenu
            break;
          case JOYSTICK_RIGHT2:
            // Show/Edit current TripLog Line
            //initializeCurrentScreen();
            break;
          }
          break;
        case TFTscreens_StartTrip:
          switch (curBUTTONSstate) {
            case JOYSTICK_LEFT2:
                screenCur=screenCurForRestore;  // after exit main menu go to screen before entering to the Trip screens
                setMenu(&menu1[0], 3); // set Log subMenu
                break;
            case JOYSTICK_RIGHT2:
                // Start Trip
                startTrip();
                screenCur=TFTscreens_TripInProgress;
                initializeCurrentScreen();
                break;
          }
          break;
        case TFTscreens_TripInProgress:
          switch (curBUTTONSstate) {
          case JOYSTICK_UP2:
            // Move pointer to the previous line on screen
            screenElementCur--;
            if(screenElementCur==0){screenElementCur=1;}
            break;
          case JOYSTICK_DOWN2:
            // Move pointer to the next line on screen
            screenElementCur++;
            // checking for upper bound is done in display routine
            break;
          case JOYSTICK_LEFT2:
            if(screenElementCur==1){if(tripReasonSelected>1){tripReasonSelected--;}}  // Reason
            else if(screenElementCur==2){if(tripDestSelected>1){tripDestSelected--;}}  // Destination
            else {exitTripScreen();}
            break;
          case JOYSTICK_RIGHT2:
            if(screenElementCur==1){if(tripReasonSelected<settingsReadByte(settingsEEPROMchip_predef_Reasons)){tripReasonSelected++;}}  // Reason
            else if(screenElementCur==2){if(tripDestSelected<settingsReadByte(settingsEEPROMchip_predef_Dest)){tripDestSelected++;}}  // Destination
            else if(screenElementCur==3){finishTrip();exitTripScreen();}  // Stop Trip
            else if (screenElementCur==4){cancelTrip();exitTripScreen();}   // Cancel trip (decrease count of log lines)
            break;
          }
          break;
       }
    }
  }

}

