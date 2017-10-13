/*Version 4.0 from 29 09 2017 with reading of parameters from the engine ECU according to ISO 14230-4 KWP 10.4 Kbaud 
for the car Citroen C5 2.0 HDI 2003 with automatic transmission */
#include <SoftwareSerial.h>
#include <DallasTemperature.h>
SoftwareSerial SIM800(7, 6);    // RX, TX
#define ONE_WIRE_BUS 2          // pin 2 as sensor connection bus DS18B20
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
/*
#include <OLED_I2C.h>           // Connect the library for OLED display
OLED  myOLED(SDA, SCL, 8);
extern uint8_t MediumNumbers[];
extern uint8_t RusFont[];
extern uint8_t SmallFont[];
*/

#define STARTER_Pin 8   // on the starter relay, via a transistor with the 8 th contact ARDUINO
#define ON_Pin 9        // on the ignition relay, via the transistor from the 9th contact ARDUINO
#define ACTIV_Pin 13    // on the LED through the transistor, from the 13th pin to indicate activity 
#define BAT_Pin A0      // to the battery, through a voltage divider of 39 kΩ / 11 kΩ
#define Feedback_Pin A1 // on the ignition wire after the relay, through the voltage divider 39 kOhm / 11 kOhm
#define STOP_Pin A2     // to the brake pedal wire, through a 39 kOhm / 11 kΩ divider
#define PSO_Pin A3      // contact for oil pressure sensor, through a 39 kOhm / 11 kΩ divider
#define RESET_Pin 5     // reset contact RES SIM800L
#define K_line_RX 0     // to 1-pin chip L9637D
#define K_line_TX 1     // to 4-pin chip L9637D
#define FIRST_P_Pin 11  // the first position of the ignition key

float TempDS;            // temperature sensor DS18B20
float Vbat;              // vehicle power supply voltage variable
float m = 66.91;         // divider for converting ADC to volts for resistors 39/11 kOm
float TempStart= -45;    // temperature at the time of last start
float VbatStart= -12;    // the voltage at the time of the last start

int Temp1 = 75;         // engine coolant temperature obtained from OBD
int Temp2 = 25;         // air temperature in the intake manifold obtained from the OBD
int PMM = 100;          // engine revs obtained from OBD
int k = 0;
int pac =0;
int poz = 0;            // position in the pin-code array
int pin[20];            // pin input array
int Timer = 0;          // countdown timer
int Timer2 = 360;       // Hour timer (60 sec. x 60 min. / 10 = 360 ) 

byte  init_obd[] = {0xC1,0x33,0xF1,0x81,0x66};      // initialization K-line     C1 33 F1 81 66 
byte temp1_obd[] = {0xC2,0x33,0xF1,0x01,0x05,0xEC}; // antifreeze temp. request  C2 33 F1 01 05 EC
byte temp2_obd[] = {0xC2,0x33,0xF1,0x01,0x0F,0xF6}; // air temperature request   C2 33 F1 01 0F F6
byte   pmm_obd[] = {0xC2,0x33,0xF1,0x01,0x0C,0xF3}; // engine speed request      C2 33 F1 01 0C F3

String at = "";
unsigned long Time1 = 0; 

bool heating = false;    // state variable warm-up mode
bool sms_report = true;  // sending SMS report enabled
bool debug = false;      // 

void setup() {
  pinMode(ACTIV_Pin, OUTPUT);    // indicate the contact on the output for the LED indication
  pinMode(ON_Pin, OUTPUT);       // indicate the pin to the output to the ignition relay
  pinMode(STARTER_Pin, OUTPUT);  // pin to the output of the starter relay
  pinMode(FIRST_P_Pin , OUTPUT); // pin to the output of the starter relay
  pinMode(RESET_Pin, OUTPUT);    // pin to the output to reboot the GSM module
  SIM800.begin(9600);            // adjust the speed of communication between Arduino and Sim 800

  if (digitalRead(STOP_Pin) == HIGH){ 
//  myOLED.begin();                   // Initialize the OLED display  
  debug = true;                       // entering the debug mode
  Serial.begin(9600);
  Serial.println("Debug mode "); 
                             } else {
  pinMode(K_line_RX, INPUT); 
  pinMode(K_line_TX, OUTPUT); 
                                    }
  SIM800_reset();                // reset the GSM modem after power-up
             }

void SIM800_reset() {            // Sim800 reboot and pre-setting function
  digitalWrite(RESET_Pin, LOW), delay(2000), digitalWrite(RESET_Pin, HIGH), delay(5000);
  SIM800.println("ATE0"),                 delay(50); // disable the echo mode
  SIM800.println("AT+CLIP=1"),            delay(50); // enable caller ID
  SIM800.println("AT+DDET=1"),            delay(50); // enable DTMF decoder
  SIM800.println("AT+CMGF=1"),            delay(50); // enable text mode for SMS 
  SIM800.println("AT+CSCS=\"GSM\""),      delay(50); // enable text encoding SMS
  SIM800.println("AT+CMGDA=\"DEL ALL\""), delay(50); // delete all SMS from memory
  SIM800.println("AT+VTD=1"),             delay(50); // specify the duration of the DTMF response
                   } 

void loop() {
  
  if (SIM800.available()) {     // if something came from the SIM800 in the direction of Arduino....
    while (SIM800.available()) k = SIM800.read(), at += char(k), delay(1); // we write into the variable at
    
    if (at.indexOf("+CLIP: \"+375290000000\",") > -1){   // <- enter your phone number
      delay(50);
      SIM800.println("ATA");                          // accept a challenge    
      pin[0]=0, pin[1]=0, pin[2]=0, poz=0, delay(50); // reset the pin code
      SIM800.println("AT+VTS=\"3,5,7\"");             // ringtone
      
    } else if (at.indexOf("+CLIP: \"+375290000001\",") > -1){ // <- enter your phone number
      delay(50);
      SIM800.println("ATH0");                         // reject a call
      Timer = 60, enginestart();                      // set the timer to 60 (600 seconds), start the engine
 // } else if (at.indexOf("+CME ERROR:") > -1){SIM800_reset();
    } else if (at.indexOf("+DTMF: 1") > -1) {pin[poz]=1, poz++; 
    } else if (at.indexOf("+DTMF: 2") > -1) {pin[poz]=2, poz++; 
    } else if (at.indexOf("+DTMF: 3") > -1) {pin[poz]=3, poz++; 
    } else if (at.indexOf("+DTMF: 4") > -1) {pin[poz]=4, poz++;
    } else if (at.indexOf("+DTMF: 5") > -1) {pin[poz]=5, poz++; 
    } else if (at.indexOf("+DTMF: 6") > -1) {pin[poz]=6, poz++; 
    } else if (at.indexOf("+DTMF: 7") > -1) {pin[poz]=7, poz++; 
    } else if (at.indexOf("+DTMF: 8") > -1) {pin[poz]=8, poz++; 
    } else if (at.indexOf("+DTMF: 9") > -1) {pin[poz]=9, poz++; 
    } else if (at.indexOf("+DTMF: 0") > -1) {pin[poz]=0, poz++;
    } else if (at.indexOf("+DTMF: *") > -1) {pin[0]=0, pin[1]=0;
      pin[2]=0, poz=0, delay(50), SIM800.println("AT+VTS=\"9\""); 
    } else if (at.indexOf("+DTMF: #") > -1) {SIM800.println("ATH0"), delay(1000), SMS_Send();}
    at = "";  // clear the variable
                           }

        if (pin[0]==1 && pin[1]==2 && pin[2]==3){pin[0]= 0, pin[1]=0, pin[2]=0, poz=0, Timer = 60, enginestart(); 
  }else if (pin[0]==4 && pin[1]==5 && pin[2]==6){pin[0]= 0, pin[1]= 0, pin[2]=0, poz=0, Timer = 120, enginestart(); 
  }else if (pin[0]==7 && pin[1]==8 && pin[2]==9){pin[0]=0, pin[1]=0, pin[2]=0, poz=0, heatingstop(); }

if (millis()> Time1 + 10000) detection(), Time1 = millis(); // perform the function detection () every 10 seconds. 
if (heating == true && digitalRead(STOP_Pin)==1) heatingstop() ; //If the brake pedal is pressed in the warm-up mode
 
}


void SMS_Send(){                                              // SMS sending function
     SIM800.println("AT+CMGS=\"+375290000002\""), delay(100); // indicate recipient's phone number
     SIM800.print("rev. SIM800L v 09.10.2017 ");
     SIM800.print("\n TempStart: "),  SIM800.print(TempStart),  SIM800.print(" grad.");
     SIM800.print("\n Temp 2 min: "), SIM800.print(TempDS),     SIM800.print(" grad.");
 //  SIM800.print("\n Temp.engine: "),SIM800.print(Temp1),      SIM800.print(" grad.");
 //  SIM800.print("\n Temp.Air: "),   SIM800.print(Temp2),      SIM800.print(" grad.");
 //  SIM800.print("\n PMM engine: "), SIM800.print(PMM),        SIM800.print(" -1/min.");
     SIM800.print("\n Timer: "),      SIM800.print(Timer/6),    SIM800.print(" min."); 
     SIM800.print("\n VbatStart: "),  SIM800.print(VbatStart),  SIM800.print(" V.");
     SIM800.print("\n Vbat 2 min: "), SIM800.print(Vbat),       SIM800.print(" V.");
     SIM800.print("\n Uptime: "), SIM800.print(millis()/3600000), SIM800.print(" H.");
     SIM800.print((char)26);                                  // end with special characters
     if (debug == true) Serial.println ("Sending sms successful.");
               }
                
void OLED_print(){
                // will be implemented later
Serial.print ("    TempDS: "), Serial.print (TempDS), Serial.print ("  ||  Vbat: "), Serial.print (Vbat);    
Serial.print (" || Timer: "),  Serial.print (Timer),  Serial.print ("  || Timer2: "), Serial.print (Timer2);   
Serial.println ("");                  
}


void detection()   {                      // perform the action every 10 seconds
  
    sensors.requestTemperatures();        // read the temperature from the sensors DS18B20
    TempDS = sensors.getTempCByIndex(0);  // temperature from the first sensor
    Vbat = analogRead(BAT_Pin);           // we measure ADC
    Vbat = Vbat / m ;                     // dividing the value of the ADC by the module, we obtain the voltage
    
    if (Timer == 48 && sms_report == true) SMS_Send();            // we send SMS 2 minutes after the start (60-6*2=48)
    if (Timer > 0 ) Timer--;                                      // we subtract one from the timer
    
    //  Autostart of the engine when the temperature drops below - 18 degrees and SMS notification at - 25.
    if (Timer2 == 2 && TempDS < -18 && TempDS != -127) Timer2 = 1080, Timer = 120, enginestart();  
    if (Timer2 == 1 && TempDS < -25 && TempDS != -127) Timer2 = 360, SMS_Send();    
        Timer2--;                                                 // we subtract one from the timer
    if (Timer2 < 0) Timer2 = 1080;                                // postpone checking for 3 hours (60x60x3/10 = 1080)
  
    if (heating == true && Timer <1) heatingstop();               // if the timer is empty, turn off the warm-up
    if (heating == true && Vbat < 10.0) heatingstop();            // if the voltage drops below 10 volts, turn off the warm-up
    if (heating == false && digitalRead(Feedback_Pin) == LOW) digitalWrite(ACTIV_Pin, HIGH), delay (30), digitalWrite(ACTIV_Pin, LOW); 
    if (debug == true) OLED_print();
                   }             
                   
 
void enginestart() {                      // engine start program

SIM800.println("AT+VTS=\"7,7\""), TempStart = TempDS, VbatStart = Vbat ;
int count = 2;           // a variable that keeps the number of remaining attempts to run
int StarterTime = 1000;  // storage variable starter operating time (1 sec for the first attempt)
if (TempDS < 15)   StarterTime = 1200, count = 2;
if (TempDS < 5)    StarterTime = 1500, count = 2;
if (TempDS < -5)   StarterTime = 2000, count = 3;
if (TempDS < -10)  StarterTime = 3000, count = 3;
if (TempDS < -15)  StarterTime = 5000, count = 4;
if (TempDS < -20)  StarterTime = 0,    count = 0, SMS_Send(); // do not even try to start 
 
 while (Vbat > 10.00 && digitalRead(Feedback_Pin) == LOW && digitalRead(STOP_Pin) == LOW && count > 0) 
 {  /* if the voltage is more than 10 volts, the ignition is switched off, the "STOP" pedal is not 
  pressed and there are attempts to start ..*/
    count--;                       // reduce by one the number of remaining attempts to run
    digitalWrite(ACTIV_Pin, HIGH), digitalWrite(FIRST_P_Pin, HIGH); // Light the LED
    digitalWrite(ON_Pin, LOW), delay (3000), digitalWrite(ON_Pin, HIGH), delay (5000); 
    
    // depending on the temperature, we again turn off and turn on the ignition (actual for the diesel engine)  
    if (TempDS < -5)  digitalWrite(ON_Pin, LOW), delay (2000), digitalWrite(ON_Pin, HIGH), delay (6000);
    if (TempDS < -15) digitalWrite(ON_Pin, LOW), delay (10000), digitalWrite(ON_Pin, HIGH), delay (8000);

    ODB_read();                                           //  read data from ODB  
    
    digitalWrite(STARTER_Pin, HIGH), delay (StarterTime); // turn on the starter relay
    digitalWrite(STARTER_Pin, LOW),  delay (7000);        // disable starter relay, wait 7 seconds
 
    ODB_read();                                           //  read the data from ODB after the start
    Vbat = analogRead(BAT_Pin);           // we measure ADC
    Vbat = Vbat / m ;                     // dividing the value of the ADC by the module, we obtain the voltage 
   
  //  if (digitalRead(PSO_Pin) == LOW)  // check the success of starting the engine speed by voltage from the sensor  /- А - /
  //  if (digitalRead(PSO_Pin) == HIGH) // check the success of starting the engine speed by voltage from the sensor  /- B - /
      if (Vbat > 12.00)                 // monitoring of starting the motor by voltage                                /- C - /
  //  if (PMM > 0)                      // check the success of starting on engine speed from OBD                     /- D - /
        { 
        heating = true, digitalWrite(ACTIV_Pin, HIGH), SIM800.println("AT+VTS=\"7,2,1,4,9\""), delay(3000);
        break;                           // we consider the start to be successful, we leave the engine start cycle
        }
    else                                                  // the start did not happen, repeat the cycle
        { 
        heating = false, digitalWrite(ON_Pin, LOW), digitalWrite(ACTIV_Pin, LOW), digitalWrite(FIRST_P_Pin, LOW);
        StarterTime = StarterTime + 200;                   // increase the time of the next start by 0.2 sec.
        SIM800.println("AT+VTS=\"8,8,8\""), delay(3000);
        }
    }
SIM800.println("ATH0");                // break the call
                  }

void heatingstop() {                   // stop function
    digitalWrite(ON_Pin, LOW), digitalWrite(ACTIV_Pin, LOW), digitalWrite(FIRST_P_Pin, LOW);
    heating= false, TempStart= -55, VbatStart = -13;   // for SMS of the report at unsuccessful start
    Timer = 0, SIM800.println("AT+VTS=\"8,8,8\""), delay (1500);
                   }





void ODB_read()    {                                            // Reading the bus K-line chip L9637D
                  /*
if (pac == 0) {
  digitalWrite(K_line_TX, HIGH), delay(300); 
  digitalWrite(K_line_TX, LOW), delay(25);   
  digitalWrite(K_line_TX, HIGH), delay(25);                     //-------------_-
  Serial.begin(10400);                                          // bus speed setting ISO 14230-4 KWP 10.4 Kbaud
  for(int i=0;i<5;i++) Serial.write(init_obd[i]), delay (10);   // send the initialization command K-line bus
  delay(100);  
  myOLED.setFont(SmallFont), myOLED.print("SEND> C1 33 F1 81 66", 0, 0), myOLED.update(); 
               }
 
  char byfer[30];
  n = Serial.available();
  if (n > 0) {  pac++;
  for (int i=0;i<n;i++) byfer[i]=Serial.read();
  myOLED.setFont(RusFont), myOLED.print("ghbyznj ,fqn", 0, 10), myOLED.printNumI(n, 80, 10);
  myOLED.print("Gfrtnjd", 0, 44), myOLED.printNumI(pac,60,44), myOLED.update(); 
  String byte8 = String(byfer[8],DEC);   // С1 (HEX) = 193 (DEC) // С1 successful response
  String byte10 = String(byfer[10],DEC); // 05 HEX = 05 DEC, 0F HEX = 15 DEC, 0C HEX = 12 DEC, 0D HEX = 13 DEC

// if  (n == 5)   myOLED.setFont(SmallFont), myOLED.print("EHO>", 35, 0), myOLED.update(); 

if  (n == 12 && byte8 ==  "193")   {  // wait for bus initialization
                                        myOLED.setFont(SmallFont), myOLED.print("83 F1 10 C1 E9 8F BD ", CENTER, 20), myOLED.update(); 
                                        Serial.flush();   
                                        for(int i=0;i<6;i++) Serial.write(temp1_obd[i]), delay (10);  
                                        delay(100);
                                         }
 if (n == 13  && byte10 ==  "5" )       { // read engine antifreeze temperature from the 12th byte of the package
                                        s = String(byfer[11],DEC);
                                        Temp1 = s.toInt() - 40;  
                                        for(int i=0;i<6;i++) Serial.write(temp2_obd[i]), delay (10); 
                                        delay(100);
                                        }

                                          
 if (n == 13  && byte10 ==  "15" )     {  // read the air temperature at the inlet of the 12th byte of the packet
                                        s = String(byfer[11],DEC); 
                                        Temp2 = s.toInt() - 40; 
                                        for(int i=0;i<6;i++) Serial.write(pmm_obd[i]), delay (10);
                                        delay(100);                          
                                       }


 if (n == 14  && byte10 ==  "12" )     {  // read the engine speed from the 12th and 13th bytes of the packet
                                        s = String(byfer[11],DEC);
                                        int h = s.toInt();
                                        s = String(byfer[12],DEC);
                                        int l = s.toInt();
                                        PMM = word(h, l)/4;
                                        for(int i=0;i<6;i++) Serial.write(speed_obd[i]), delay (10);
                                        delay(100);  
                                        }
                                          
  if (n == 13  && byte10 ==  "13" )    {  // if we read the speed from the 12th byte of the packet
                                        s = String(byfer[11],DEC);
                                        SPEED = s.toInt();


  myOLED.clrScr();
  myOLED.setFont(MediumNumbers);
  myOLED.printNumI(Temp1, RIGHT, 0), myOLED.printNumI(Temp2, RIGHT, 16), myOLED.printNumI(PMM, RIGHT, 32), myOLED.printNumI(SPEED, RIGHT, 48);
  myOLED.setFont(RusFont);
  myOLED.print("NTVGTHFNEHF LDBU", LEFT, 0), myOLED.print("NTVGTHFNEHF DJPL", LEFT, 20),myOLED.print("J<JHJNS LDBU", LEFT, 36), myOLED.print("CRJHJCNM FDNJ", LEFT, 54);
  myOLED.update();
                                       }
                                          
        }
        */
                 }
