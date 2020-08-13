//Mapping address from PLC
//  Name            Address Size
//  REGISTER NO.      0       1
//  PART NO.          2       7
//  MODEL             10      5
//  TUBE LENGTH       16      2 DW
//  CUTTING LEGTH     18      2 DW
//  MATERIAL NO.      20      5
//  SPEED SETTING     26      2 DW
//  TYPE OF PRODUCE   40      1
//  NEXT PROCESS      41      1
//  QTY               42
//  YY                43
//  MM                44
//  DD                45
//  HHMM              46
//  
//  QR Triger         49.0
//  QR DATA           50
//
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#include <TimerOne.h>                           // Header file for TimerOne library
#define TIMER_US 100000                         // 100mS set timer duration in microseconds 

#include <SoftwareSerial.h>
#include "thermalprinter.h"
#define ledPin 13
#define rxPin 18
#define txPin 19
int printStatus = 0;
Epson TM82 = Epson(rxPin, txPin); // init the Printer with Output-Pin

// for loadcell force
//#include <EEPROM.h>             //inbox
//#include <HX711.h>              //BOGDE HX711 from Github https://github.com/bogde/HX711 !do not use other librarie! as code may fail!
// for i/o expansion
//#include <Wire.h>
//#include "Adafruit_MCP23017.h"


#include "Mudbus.h"
#include "SD.h"
#include "FINS.h"

Mudbus Mb;
//Function codes 1(read coils), 3(read registers), 5(write coil), 6(write register)
//signed int Mb.R[0 to 125] and bool Mb.C[0 to 128] MB_N_R MB_N_C
//Port 502 (defined in Mudbus.h) MB_PORT

FINS OmronFins;
// Port 9600


//Adafruit_MCP23017 mcp1;
//Adafruit_MCP23017 mcp2;

// The circuit:
// * SD card attached to SPI bus as follows:
// ** MOSI - pin 11
// ** MISO - pin 12
// ** CLK - pin 13
// ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)
const int chipSelect = 4;

unsigned int main_loop_counter;
unsigned int cnt;
unsigned int timer_1;

char QrData[80];
char TextData[25];

void setup()
{
  uint8_t mac[]     = { 0x90, 0xA2, 0xDA, 0x00, 0x51, 0x18 };
  uint8_t ip[]      = { 192, 168, 180, 18 };
  uint8_t gateway[] = { 192, 168, 180, 1 };
  uint8_t subnet[]  = { 255, 255, 255, 0 };

  Serial.begin(115200);
  delay(5000);
  Ethernet.begin(mac, ip, gateway, subnet);
  //Avoid pins 4,10,11,12,13 when using ethernet shield
  //define digital input for EOL tester function check
  // D2, D3, D5, D6, D7 and D8
  //pinMode(2, INPUT);
  //pinMode(3, INPUT);
  //pinMode(5, INPUT);
  //pinMode(6, INPUT);
  //pinMode(7, INPUT);
  //pinMode(8, INPUT);

//   // see if the card is present and can be initialized:
//  if (!SD.begin(chipSelect)) {
//    Serial.println("Card failed, or not present");
//    // don't do anything more:
//    return;
//  }
//  Serial.println("card initialized.");
//  File dataFile = SD.open("datalog.txt");
//
//  // if the file is available, write to it:
//  if (dataFile) {
//    while (dataFile.available()) {
//      Serial.write(dataFile.read());
//    }
//    dataFile.close();
//  }
//  // if the file isn't open, pop up an error:
//  else {
//    Serial.println("error opening datalog.txt");
//  }

   //server.begin();
   //pinMode(9, OUTPUT); //pin selected to control
   
   OmronFins.begin(0x00, ip[3], 0x00);//node id = 18
//   Serial.println("Omron run");


  TM82.start();
  
  Timer1.initialize(TIMER_US);                  // Initialise timer 1
  Timer1.attachInterrupt( timerIsr );           // attach the ISR routine here
  
} 

unsigned long new_cycle_time;
unsigned long old_cycle_time;
unsigned long cur_cycle_time;
unsigned long tm;
int           old_Triger, i;
int           SampleID;

void loop()
{
  new_cycle_time = millis();
  if( new_cycle_time != old_cycle_time ){
    cur_cycle_time = new_cycle_time - old_cycle_time;
    old_cycle_time = new_cycle_time;    
  }
  
  Mb.Run();
  //Analog inputs 0-1023
  //pin A0 to Mb.R[0]
  //Mb.R[0] = analogRead(A0); //909 = 11.8
  //Mb.R[1] = analogRead(A1); //909 = 11.8
  //Mb.R[2] = map(analogRead(A2),0,865,0,1180); //865 = 11.8
  //Mb.R[3] = map(analogRead(A3),0,865,0,1180); //865 = 11.8
  //Mb.R[4] = map(analogRead(A4),0,865,0,1180); //865 = 11.8
  //Mb.R[5] = map(analogRead(A5),0,865,0,1180);
  //bitWrite( Mb.R[6], 0, !digitalRead(2));
  //bitWrite( Mb.R[6], 1, !digitalRead(3));
  //bitWrite( Mb.R[6], 2, !digitalRead(5));
  //bitWrite( Mb.R[6], 3, !digitalRead(6));
  //bitWrite( Mb.R[6], 4, !digitalRead(7));
  //bitWrite( Mb.R[6], 5, !digitalRead(8));

  //read load cell
  //Mb.R[7] = 0;
  //Mb.R[8] = 0;
  //Mb.R[9] = timer_1;  //read force triger form plc
  

  main_loop_counter++;
  //Mb.R[10] = main_loop_counter;
  //Mb.R[12] = cur_cycle_time & 0xFFFF;
  //Mb.R[13] = cur_cycle_time>>16;

  //Inputs();
  //Outputs();
  OmronFins.Run(&Mb.R[0]);

  if(Mb.R[49]>0) {
    GenQrData();
    //print_qr();
    PagePrint();
   
    Mb.R[49]=0;
  }
}

// --------------------------
// timerIsr() 100 milli second interrupt ISR()
// Called every time the hardware timer 1 times out.
// --------------------------
void timerIsr()
{
  timer_1++;
}

void ClearTextData() {
int i;
  for(i=0;i<25;i++){
      TextData[i]  = 0;
  }
}

void setTextData(int sAddr, int n) {
int i;
  ClearTextData();
  for(i=0;i<n;i++){
      TextData[(i*2)]  = lowByte(Mb.R[i+sAddr]);  //modbus[sAddr]
      TextData[(i*2)+1] = highByte(Mb.R[i+sAddr]);  //modbus[sAddr]
  }
}

void ClearQrData() {
int i;
  for(i=0;i<60;i++){
      QrData[i]  = 0x20;
  }
}

void getQrData() {
int i;
  ClearQrData();
  for(i=0;i<20;i++){
      QrData[(i*2)]  = highByte(Mb.R[i+50]);  //modbus[50]
      QrData[(i*2)+1] = lowByte(Mb.R[i+50]);  //modbus[50]
  }
}

char cnvBCD2Char(int bcd) {
  switch(bcd) {
    case 0: return('0');
    case 1: return('1');
    case 2: return('2');
    case 3: return('3');
    case 4: return('4');
    case 5: return('5');
    case 6: return('6');
    case 7: return('7');
    case 8: return('8');
    case 9: return('9');
  }
}

void GenQrData() {
int i;
  ClearQrData();
  //"Part No..........: "
  setTextData(2, 7);
  QrData[0]= TextData[0];//1
  QrData[1]= TextData[1];
  QrData[2]= TextData[2];//2
  QrData[3]= TextData[3];
  QrData[4]= TextData[4];//3
  QrData[5]= TextData[5];
  QrData[6]= TextData[6];//4
  QrData[7]= TextData[7];
  QrData[8]= TextData[8];//5
  QrData[9]= TextData[9];
  QrData[10]= TextData[10];//6
  QrData[11]= TextData[11];
  QrData[12]= TextData[12];//7
  QrData[13]= TextData[13];
  //"Mat. No..........: "
  setTextData(20, 5);
  QrData[14]= TextData[0];//1
  QrData[15]= TextData[1];
  QrData[16]= TextData[2];//2
  QrData[17]= TextData[3];
  QrData[18]= TextData[4];//3
  QrData[19]= TextData[5];
  QrData[20]= TextData[6];//4
  QrData[21]= TextData[7];
  QrData[22]= TextData[8];//5
  QrData[23]= TextData[9];
  //YYMMDDHHmm
  QrData[24]= cnvBCD2Char(lowByte(Mb.R[43])>>4);//YY
  QrData[25]= cnvBCD2Char(lowByte(Mb.R[43])&0x0F);
  QrData[26]= cnvBCD2Char(lowByte(Mb.R[44])>>4);//MM
  QrData[27]= cnvBCD2Char(lowByte(Mb.R[44])&0x0F);
  QrData[28]= cnvBCD2Char(lowByte(Mb.R[45])>>4);//DD
  QrData[29]= cnvBCD2Char(lowByte(Mb.R[45])&0x0F);
  QrData[30]= cnvBCD2Char(highByte(Mb.R[46])>>4);//HH
  QrData[31]= cnvBCD2Char(highByte(Mb.R[46])&0x0F);
  QrData[32]= cnvBCD2Char(lowByte(Mb.R[46])>>4);//MM
  QrData[33]= cnvBCD2Char(lowByte(Mb.R[46])&0x0F);
}

void print_qr() {
  //print header
  TM82.justifyLeft();
  TM82.boldOn();
  TM82.println("................... Denso ...................");  
  TM82.boldOff();
  // print QRcode
  TM82.qrSelectModel();
  TM82.qrSetSize(4);
  TM82.qrErrorM();
  TM82.qrStoreData(10);
  TM82.println(QrData);  
  TM82.qrPrint();
  TM82.defaultLineSpacing();  
  TM82.println(QrData);  


  //foot
  TM82.cut();  
}

void PagePrint() {
  TM82.StandatdMode();
  TM82.justifyLeft();
  TM82.PageMode();
  TM82.SetMotion();
  TM82.SetPrnArea();
  //---- header ----
  TM82.boldOn();
  //TM82.println("12345678901234567890123456789012345678901234567890");  
//  TM82.SetXPos(0);  
//  TM82.SetYPos(10);  
  TM82.println("WELLGROW PLANT");  
  TM82.println("27D TUBE BRAZING M/C");  
  TM82.println("*IN-HOUSE PART TAG");  
  TM82.boldOff();
  TM82.println("------------------------------------------------");  
  TM82.boldOn();
  //          12345678901234567890
  TM82.SetXPos(120);  
  TM82.print("Type of Product..: ");
  TM82.boldOff();
  if(Mb.R[40]==1) {
    TM82.println("GSR.");  
  }else if(Mb.R[40]==2) {
    TM82.println("TYPE 2");  
  }else {
    TM82.println("");  
  }
  
  TM82.boldOn();
  TM82.SetXPos(120);  
  TM82.print("Part No..........: ");
  TM82.boldOff();
  setTextData(2, 7);
  TM82.println(TextData);  
  //TM82.SetXPos(0);  
  //TM82.SetYPos(200);  
  TM82.boldOn();
  TM82.SetXPos(120);  
  TM82.print("Mat. No..........: ");
  TM82.boldOff();
  //TM82.SetXPos(500);  
  //TM82.SetYPos(200);  
  setTextData(20, 5);
  TM82.println(TextData);  

  //TM82.SetXPos(0);  
  //TM82.SetYPos(300);  
  TM82.boldOn();
  TM82.SetXPos(120);  
  TM82.print("Prod.Date........: ");
  TM82.boldOff();
  TM82.print(QrData[24]);
  TM82.print(QrData[25]);
  TM82.print('/');
  TM82.print(QrData[26]);
  TM82.print(QrData[27]);
  TM82.print('/');
  TM82.print(QrData[28]);
  TM82.print(QrData[29]);
  TM82.print(' ');
  TM82.print(QrData[30]);
  TM82.print(QrData[31]);
  TM82.print(':');
  TM82.print(QrData[32]);
  TM82.print(QrData[33]);
  TM82.println("");
 
  //TM82.SetXPos(0);  
  //TM82.SetYPos(400);  
  TM82.boldOn();
  TM82.SetXPos(120);  
  TM82.print("Q,TY.............: ");
  TM82.boldOff();
  if( Mb.R[42]==0 ){
    Mb.R[42] = 400;
  }
  TM82.println(Mb.R[42],DEC);

  //TM82.SetXPos(0);  
  //TM82.SetYPos(500);  
  TM82.boldOn();
  TM82.SetXPos(120);  
  TM82.print("Next Process.....: ");
  TM82.boldOff();
  if(Mb.R[41]==0) {
    TM82.println("OEM");  
  }else {
    TM82.println("EXPORT");  
  }
  //print text xy
  //TM82.SetXPos(250);  
  //TM82.SetYPos(80);  
  TM82.println(QrData);  

  //---- body -----
  // print QRcode
  TM82.SetXPos(0);  
  TM82.SetYPos(100);//80  
  TM82.qrSelectModel();
  TM82.qrSetSize(4);
  TM82.qrErrorM();
  TM82.qrStoreData(40);
  TM82.println(QrData);  
  //TM82.println("123456789012345678901234567890");
  TM82.qrPrint();
  //print

  TM82.BatchPrint();
//  TM82.StandatdMode();
  TM82.cut();  
}

