/*************************************************** 
* Global variables, structures etc.                *
****************************************************/
// This buffer is shared by different functions (to save RAM)
#define globalBuffer_size 80
char gBuff[globalBuffer_size]; // global buffer 

/*************************************************** 
* Menu routines variables and structures           *
****************************************************/
#define lengthMenuLineStr 20

// undefined - not in use (just for information
// qntItmMenu - number of lines in sub menu
// addr - address of data of submenu
typedef struct mnuHdr{
  byte undefined, qntItmMenu; 
  unsigned int addr;  //struct mnuLines * firstLine[];
};

// idMr, idMl - action if go right or left in the menu
//    0xFF - exit from menu
//    0x00 - run menu command function
//    0xnn - show menu with number NN (from mnuHdr)
typedef struct mnuLines{
  byte idMr, idMl;  // commands to go (left / right). 0 do command
  char lineStr[lengthMenuLineStr];
};

mnuLines PROGMEM menu1sub1[]={
  {0xFF,0x00, "Main Screen"},
  {0xFF,0x03, "Trip Log"},
  {0xFF,0x00, "Temperature"},
  {0xFF,0x00, "Speed & motor load"},
  {0xFF,0x02, "Settings"}
};

mnuLines PROGMEM menu1sub2[]={
  {0x01,0x00, "Receive BT data"},
  {0x01,0x00, "Odometer"},
  {0x01,0x00, "Default Trip Names"},
  {0x01,0x00, "Time/Date"}
};

mnuLines PROGMEM menu1sub3[]={
  {0x01,0x00, "Start/End Trip"},
  {0x01,0x00, "View Log"},
  {0x01,0x00, "View Totals"},
  {0x01,0x00, "Download Log"}
};

// first byte here just for information (shows submenu number
mnuHdr PROGMEM menu1[] = {
  {1,sizeof(menu1sub1)/sizeof(mnuLines), (unsigned int)&menu1sub1},
  {2,sizeof(menu1sub2)/sizeof(mnuLines), (unsigned int)&menu1sub2},
  {3,sizeof(menu1sub3)/sizeof(mnuLines), (unsigned int)&menu1sub3}
};

byte menuCurSelLine;  // current selected menu line (use also in editing lines). 0 means nothing selected
byte menuCur=0;  // current selected menu. 0 means no menu showed.
unsigned int menuAddr;  // Address of menu definition. Allows to use different menu definitions


/*************************************************** 
* SCREEN variables and constants                   *
****************************************************/
#define TFTscreens_MainScreen     1
#define TFTscreens_StartTrip      2
#define TFTscreens_TripInProgress 3
#define TFTscreens_Temperature    4
#define TFTscreens_SpeedMotor     5
#define TFTscreens_ViewTripLog    6
#define TFTscreens_ViewTotalsLog  7
#define TFTscreens_DownloadLog    8

byte screenCur=0;  // Current screen displayed on TFT
byte screenCurForRestore;   // for store screenCur value if need to restore it later (in Trip log)


/*************************************************** 
* Speed Gauge and Motor load Drawing data          *
****************************************************/
// color table for bars in PGMSPACE
byte PROGMEM speedNumCoordTable[]={
  26+12, 209,// 0
  22, 164,   // 20
  36, 122,   // 40
  66, 89,    // 60
  108, 71,   // 80
  152, 72,   // 100
  193+4, 90,   // 120
  223, 124,  // 140
  236, 166,  // 160
  232-2, 211 // 180
};

byte prevEngineLoadValue=0; // for achieving smooth transition on TFT between motor load changes (in %)

/*************************************************** 
* OBD related data                                 *
****************************************************/
#define TimeGap_AvgSpeedReadCnt 3 // Count for average speed readings for the gap (10 means 5 seconds, if OBDReadSpeedInterval=500ms)
#define DistanceCorrCoef 1.096  // correction coefficient for calculating distance (must figure out by try)
const char PROGMEM OBDdataCMD[][5] = {"010D","0104"};
byte curOBDcmd=0;    // counter for OBDdataCMD array. Read one by one commands and send them to the OBD
float TraveledDistance = 0; // Distance traveled (calculated from speed) - reflects only distance from last save to EEPROM.
float CurrentSpeed=0;  //  current speed for display on TFT
byte MotorLoad=0;  // stores motor load value for display on TFT


/*************************************************** 
* BlueTooth related data                           *
****************************************************/
#define BT_ResetPin  7
#define BT_PIO11Pin  4


/*************************************************** 
* RTC related data                                 *
****************************************************/
#define correctRTCevery 86400L  // correct RTC clock every 24h (86400L seconds)
#define correctRTCvalue 16  // 16 seconds


/*************************************************** 
* Walk icon related data                           *
****************************************************/
#define walking_Speed 180 // ms
#define walking_bitmapSize 55   // bytes for every bitmap in GIF
byte walkingCounter;   // 0 - no walk icon, >0 speed of walk
unsigned long walkingMillis=0;  // store millis value


/*************************************************** 
* JOYSTICK related data                            *
****************************************************/
#define JOYSTICK_PINX A1
#define JOYSTICK_PINY A0
#define JOYSTICK_CENTERXmin 550
#define JOYSTICK_CENTERXmax 750
#define JOYSTICK_CENTERYmin 550
#define JOYSTICK_CENTERYmax 750
#define JOYSTICK_UPmax  900
#define JOYSTICK_DOWNmax  350
#define JOYSTICK_LEFTmax  350
#define JOYSTICK_RIGHTmax  900

#define JOYSTICK_CENTER 0
#define JOYSTICK_UP1  1
#define JOYSTICK_UP2  2
#define JOYSTICK_DOWN1  4
#define JOYSTICK_DOWN2  8
#define JOYSTICK_LEFT1  16
#define JOYSTICK_LEFT2  32
#define JOYSTICK_RIGHT1  64
#define JOYSTICK_RIGHT2  128


/*************************************************** 
* EEPROM related data                              *
****************************************************/
#define settingsEEPROMchip 0x50     // 4K
#define bitmapsEEPROMchip  0x51     // 16K (in the future will be 64K)
#define logsEEPROMchip     0x52     // 64K

// map of EEPROM chips data
// ***** settingsEEPROMchip 0x50 (4K)
#define settingsEEPROMchip_OdometerValue       0   // (4 bytes) Long data type for storing Odometer value in meters
#define settingsEEPROMchip_TripStarted         4   // (1 byte) 0 - no trip logging; 1 - Trip is started (last line in Log is editable)
#define settingsEEPROMchip_TotalLinesInLog     5   // (2 bytes) 0 - no lines, EEPROM is empty. If last trip is not finished, it is already in this quantity.
// space for another small data
#define settingsEEPROMchip_predef_Reasons      1024 // (401 bytes) List of predefined reasons of trip (each line 40 bytes long, plus one byte - number of lines used
#define settingsEEPROMchip_predef_Dest         1425 // (401 bytes) List of predefined destinations of trip (each line 40 bytes long, plus one byte - number of lines used
#define settingsEEPROMchip_printTripProgress   1826 // (176 bytes) Draw/Print initial data on Trip data screen
#define settingsEEPROMchip_printTripProgress1  2002 // (36 bytes) Date coordinates
#define settingsEEPROMchip_printTripProgress2  2038 // (36 bytes) ODO start coordinates
#define settingsEEPROMchip_printTripProgEdit1  2074 // (141 bytes) Reason, Destination, Finish, Delete (fields and commands)

// first available byte: 2215

// ***** bitmapsEEPROMchip 0x51 (16K)
#define bitmapsEEPROMchip_tempTop           0   // (684 bytes)
#define bitmapsEEPROMchip_tempWindow      684   // (1084 bytes)
#define bitmapsEEPROMchip_tempBottom     1768   // (904 bytes)
#define bitmapsEEPROMchip_tempMirror     2672   // (544 bytes)
#define bitmapsEEPROMchip_mazdaLogo      3216   // (5004 bytes)

#define bitmapsEEPROMchip_xBigNumbers    8220   // (2702 bytes) Set of Extra Big numbers bitmaps. Two first bytes describe width and height of symbol.
#define bitmapsEEPROMchip_BigNumbers    10922   // (1282 bytes) Set of Big numbers bitmaps. Two first bytes describe width and height of symbol.
#define bitmapsEEPROMchip_OBDicon       12204   // (55 bytes) OBD icon bitmap
#define bitmapsEEPROMchip_BTexternal    12259   // (55 bytes) Icon of external BT device (smartphone)
#define bitmapsEEPROMchip_10x15Numbers  12314   // (302 bytes) Numbers 10x15 pixels in size
#define bitmapsEEPROMchip_11x17Numbers  12616   // (342 bytes) Numbers 11x17 pixels in size
#define bitmapsEEPROMchip_13x20Numbers  12958   // (402 bytes) Numbers 13x20 pixels in size
#define bitmapsEEPROMchip_WalkingGIF    13360   // (330 bytes)
// first available byte: 13690

// no picture yet
#define bitmapsEEPROMchip_CurItemList    9999

/*************************************************** 
* View Lists related variables                     *
****************************************************/
int listItemsTotal;     // stores a total lines of list for moving thru list
int listItemCur;        // stores a current line number of a list 
byte listItemOnScreen;  // stores a current line number  on a screen (for selecting)


/*************************************************** 
* Trip Log related data                            *
****************************************************/
byte tripReasonSelected;    // Selected line number from EEPROM list of Trip Reasons (starts from 1)
byte tripDestSelected;      // Selected line number from EEPROM list of Trip Destinations (starts from 1)
unsigned long tripStartODO; // for storing Start ODO value to display it on TFT

#define logsEEPROMchip_sizeLine  128    // total bytes for everzy line
#define logsEEPROMchip_LineDate    0      // (3 bytes) Log line starts with Date
#define logsEEPROMchip_LineBegin   3      // (4 bytes) Unsigned long - Start km value (meeters)
#define logsEEPROMchip_LineFinish  7      // (4 bytes) Unsigned long - End km value (meeters)
#define logsEEPROMchip_LineDest   11      // (40 bytes) Destination
#define logsEEPROMchip_LineReason 51      // (40 bytes) Reason for driving
#define logsEEPROMchip_LastByte   91      // we need this line for calculations
/*Logger data in 64K EEPROM (91 bytes per line, lets reserve 128 (page size)):
[DATE 3b]		231213 (d/m/y)
[BEGIN KM 4b]		567849
[FINISH KM 4b]		567889
[DESTINATION 40b]	Diez
[REASON 40b]		Zusamenkonf
[RESERVED 37b]		
*/


/*************************************************** 
* Screen elements for printing                     *
****************************************************/
byte screenElementCur;  // variable for storing cursor position.
#define printEEPROMstructure_lineSize 35
/* structure of data in EEPROM for printing elements on screen (35 bytes per line)
This structure is helpful for printing some words, lines or whatever on the screen in certain position.
Also this structure can be used to print and edit the data on the screen (for Trip routines for example)
(1) number of elements in structure (last bit in byte shows a cursor present)
-- line data
(1) element type (Text (1); List in EEPROM (2))
(2) X top corner of area on screen
(2) Y top corner of area on screen
(2) width in pixels of area on screen
(2) height in pixels of area on screen
(2) Fcolor of printed data
(2) Bcolor of printed data
(1) Bold parameter of printed data
(21) data (EEPROM address (2 bytes for list address); Text (zero terminated); address of RAM buffer where number is stored). remaining bytes fill with 0.

Element type variations:
    1 - data contains a 0-terminated string, copied to global buffer for printing. But later, in edit data routine will work as a text for a command
    2 - Print text stored in other settings EEPROM location (useful if text is greather than 20 symbols)
    3 - Draw a rectangle (Fcolor)
    4 - Fill a rectangle (Bcolor)
    5 - (not yet programmed)
Function printEEPROMstructure(addr) prints all elements from structure in EEPROM addressed by addr
*/







/*
General Data:
data format:
4 bytes - Odometer value (in meeters)
1 byte - seconds to add to RTC time every [RTC Correction Span].
4 bytes - RTC Correction Span (seconds between RTC correction). default is 24h.
Write EEPROM when speed is 0 and data is newer that in EEPROM.
(Other trigger can be: When ignition key is turned off (or motor rpm is 0).)

Log data:
data format:
Predefined REASONS for driving:
1 byte - Total predefined lines used (now is 4).
40 bytes per line. Total allow for 10 lines. Currently used 4:
Predigtdienst
Zusammenkunft
Predigtdienst/Zusammenkunft
Versammlungsarbeit

Predefined DESTINATIONS for driving
1 byte - Total predefined lines used (now is 3).
40 bytes per line. Total allow for 10 lines. Currently used 3:
Hahnstatten
Linter
Diez

BT config:
14 bytes - Address of OBD BT (in text format)
6 bytes - PIN of OBD BT

DS temperature sensors (2):
8 bytes per sensor 1 address
8 bytes per sensor 2 address




Data on Screen:
View Log:

1234567890123456789012345678901234567890
1  01.12.13 45km - Reason
2  04.12.13 40km - Reason
3  07.12.13 40km - Reason
4  11.12.13 52km - Reason
5 >14.12.13 39km - Reason
6  18.12.13 40km - Reason
7  21.12.13 60km - Reason
8
9
0
1234567890123456789012345678901234567890
*/
