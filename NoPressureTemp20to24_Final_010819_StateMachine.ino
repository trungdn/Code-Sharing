/*
* MaxQ Research Active Logger
* Purpose: This design is intended to be installed in MaxQ boxes for temperature controlled shipping containers.
* Hardware will be based around the ATMega644p processor with Arduino Bootloader, the SHT1C temperature and humidity sensor, 
* a 1.3 OLED screen, Real time clock, Hall Effect sensor, microSD card, and BLE module (bluefruit), Zigbee.
* This code controls the processor which will have a front menu to select the various settings or start a log. Inside of a log, the data is taken from the sensors, printed on the OLED screen, saved to the SD card, and sent across BLE to a mobile app.
* Trung Nguyen - MaxQ Research LLC
*/

//Libraries
#include <Arduino.h>
#include <SD.h> //SDcard lib
#include <RTClib.h> //Real Time Clock
//#include <SHT2x.h> //Temp sensor library
#include <Wire.h>    //I2C Interface    
#include <SPI.h> //SPI Interface            
#include "U8glib.h"  //OLED 
//#include <U8g2lib.h> //Testing new OLED library
#include "LowPower.h" //Low power
#include "MaxQ.h" // MaxQ libraries


typedef enum { STATE_SAMPLE, 
               STATE_TIME, 
               STATE_LOG,
               STATE_INIT,
               STATE_BLE,
               STATE_WAKEUP_LID,
               STATE_WAKEUP_DUR} states;

states state = STATE_SAMPLE;
static unsigned int timer_state;

//PinChange Interrupt
//#include <PinChangeInt.h>
//#include <PinChangeIntConfig.h>

//Define Output PIN
#define   LOGBUTTONPIN    14          //Push button for selecting and logging
#define   BREECH          11          //Digital pin to detect hall effect (when magets nearby)
#define   LEDSELECT       19          //LED when logging GreenLED
#define   LEDONOFF        15          //LED when ON/OFF RedLED
#define   MINTEMP         1           //minimum temperature for blood
#define   MAXTEMP         6           //maximum temperature for blood
#define   LOWBATTERY      A2          //Define battery percentage
#define   ALARM           A1          //Buzzer when open lid
#define   UP              12          //Up button
#define   DOWN            13          //Down button
#define   BLUETOOTH       10          //Bluetooth button
#define   BLEWAKEUP       2           //Bluetooth wake up deep sleep
#define   CHARGER         3           //Detect charger 

//Value for brightness of OLED 1.3
#define BRIGHTNESS 0x01               //OLED Brightness
#define BRIGHTNESSREG 0x81            //OLED Constract Address


//SofwareSerial to receive String from Wireless Sensor
#include <SoftwareSerial.h>
SoftwareSerial zigBee(20, 21);


//Define variables
boolean button_was_pressed;            // previous state of the log button
unsigned int refresh_rate = 120000;    // user input in milliseconds for time between samples     
int sample_rate = refresh_rate / 1000; //sample rate in seconds 
unsigned long timeStep=0;       // Variable for time step increase and printing
//float humid;              //Optional humidity as a percentage
int badBlood;         //variable to show when blood has gone out of bounds
float timeOpen;         //Start time of open
float timeClose;     //Time close
int menuPos = 15;          // value to show where the menu arrow is to be pointed
int timerPos = 40;
uint8_t page = 0;

/**** Allocation of I2C Address for RTC ****/              
RTC_PCF8523 rtc;   // Allocating RTC address

/**** Defining RTC Variables ****/
DateTime        now;
long unixtime;
static uint16_t year;
static uint8_t  month, day, hour, minute, second;

/**** Defining Datafile Variables ****/
// offsets:    0123456789012
byte fileName[] = "mmddyyxx.CSV";
File dataFile;
byte log_is_open;


//Variables Zigbee
bool started= false;//True: Message is strated
bool ended  = false;//True: Message is finished 
char incomingByte ; //Variable to store the incoming byte
char msg[9];    //Message example <24.21>
byte indexStr;     //Index of array


/**** SD Card Setup ****/
const int CS_pin = 4;  //We always need to set the CS Pin


const unsigned char logo [] PROGMEM = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xF9, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE0, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x80, 0x08, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x01, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xE0, 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xF0, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79, 0xFF, 0xFC, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x7F, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x1F, 0x80, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x02, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0xC0, 0x1C, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xF0, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFD, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0xC0, 0x7C, 0x07, 0x80, 0xF8, 0x1E, 0x07, 0xE0, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0xC0, 0xFC, 0x0F, 0xC0, 0x7C, 0x3C, 0x1F, 0xF8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0xE0, 0xFC, 0x0F, 0xC0, 0x3E, 0x78, 0x30, 0x0C, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0xF1, 0xFC, 0x1F, 0xE0, 0x1E, 0xF8, 0x60, 0x06, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0xFB, 0xFC, 0x1F, 0xE0, 0x0F, 0xF0, 0xC0, 0x06, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0xFF, 0xFC, 0x3C, 0xF0, 0x07, 0xE0, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0xBF, 0xFC, 0x3C, 0xF0, 0x07, 0xC0, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0x9F, 0x7C, 0x78, 0x78, 0x07, 0xE0, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0x8E, 0x7C, 0x7F, 0xF8, 0x0F, 0xF0, 0xC0, 0x66, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0x84, 0x7C, 0xFF, 0xFC, 0x1E, 0xF8, 0x60, 0x76, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0x80, 0x7C, 0xFF, 0xFC, 0x3C, 0x7C, 0x70, 0x1C, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0x80, 0x7D, 0xE0, 0x3E, 0x7C, 0x3C, 0x3C, 0x3E, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0x80, 0x7D, 0xE0, 0x1E, 0xF8, 0x1E, 0x0F, 0xE3, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Select the OLED */
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);  // 1.3' I2C OLED 128x64 SH1106 

void setup() 
{
  Serial.begin(9600); //Serial begin at 9600 to send it to BLE
  Wire.begin(); //Intial for I2C OLED
  zigBee.begin(9600); ////Serial begin at 9600 to send it to receive data from Zigbee
  setOLED(); //Setting for the OLED 
  clearOLED(); //Clear the screen memory
   
  //Turn RED LED on for 3 seconds to make sure the device is on
  pinMode(LEDONOFF, OUTPUT);    //Red LED
  digitalWrite(LEDONOFF, HIGH);   // turn the LED on 
  delay(3000);              // wait for 3 seconds
  digitalWrite(LEDONOFF, LOW);    // turn the LED off 
  
  //Initialize pins/variables/hardware/interrupts
  button_was_pressed = false;
  pinMode(LEDSELECT, OUTPUT);   //Green LED
  pinMode(LOGBUTTONPIN, INPUT);     //Logging button
  digitalWrite(LOGBUTTONPIN, HIGH);         // pull-up enabled
  pinMode(BREECH, INPUT);     //Hall Effect Sensor Pin  
  digitalWrite(BREECH, HIGH); // pull-up enabled
  pinMode(UP, INPUT);       //Up button
  digitalWrite(UP, HIGH);  //pull-up enabled
  pinMode(DOWN, INPUT);   //Down button
  digitalWrite(DOWN, HIGH);  //pull-up enabled
  pinMode(BLUETOOTH, OUTPUT);  //Bluetooth Interrupt Pin
  digitalWrite(BLUETOOTH, LOW); //set to low
  pinMode(BLEWAKEUP, INPUT);  //Bluetooth wake up pin
  digitalWrite(BLEWAKEUP, LOW); //pull-down enabled

  //Reset the time open/close the lid
  timeOpen =0;
  timeClose =0;
  
  //****Interrupt 
  //attachInterrupt(digitalPinToInterrupt(BLEWAKEUP),wakeUp,RISING);
  //attachInterrupt(digitalPinToInterrupt(BREECH),wakeUp,LOW);
  attachInterrupt(digitalPinToInterrupt(BLUETOOTH),bluetoothISR,RISING);
  //PCintPort::attachInterrupt(BLUETOOTH, bluetoothISR,RISING); // attach a PinChange Interrupt to our pin on the rising edge
  
  
  drawLogo();
 //Inital Setup
  rtc_init();                             // calling RTC initialization function
  sd_init();                              // calling SD card initialization function
  SdFile::dateTimeCallback(fileDateTime);
  checkValue();
  checkFile();
  batteryLevel();
  //setMenuText(15);
  //tcaselect(3); //If using multiple temp sensors
  timer_state = 1;
}


//Interupt function to send file contents over bluetooth
void bluetoothISR()
{
  //don't reopen the file if we are not currently logging
  boolean reopen = false;
  //check if we have a file open and are currently logging to determine if we should reopen
  if(log_is_open == 1)
  {
    reopen = true;
    dataFile.close();
  }
  //open the last file to read
   dataFile=SD.open((char *)fileName);
   if (dataFile)
   {
    //delay(1000);
    //print $$ to notify the app that we are sending a file which should open a new graph and plot all this data.
    Serial.println("$$");
    //loop through every byte of data and printit.
    while(dataFile.available())
    {
      Serial.write(dataFile.read());
      //delay in microseconds to buffer output, can't do delay() because we are in an interupt
      //delayMicroseconds(13000);
    }
    //print ## to show app that the file has ended and should now plot all the data received
    Serial.println("##");
    dataFile.close();
   }
   //reopen the file and continue logging real time if it was open before.
   if(reopen == true)
   {
     dataFile = SD.open((char *)fileName, FILE_WRITE);
   }
   //delayMicroseconds(16382);
   //delayMicroseconds(16382);
}

//function for setting each unit of the date with a min and max value
int setUnit(int unit, int lowest, int highest)
{
  //state = analogRead(A3);
  if(digitalRead(DOWN)==LOW)
  {
    if(unit >= lowest)
      unit = unit - 1;
  }
  else if(digitalRead(UP)==LOW)
  {
    if(unit <= highest)
      unit = unit + 1;
  }
  delay(200);
  return unit;
}

//clock to set the text when changing time based on currently set variables
void setClockMenuText(int y, int m, int d, int h, int mi, String change, String skip, int pos)
{ 
    String d1 = String(y) + "-" + String(m) + "-" +String(d) + " " + String(h) + ":" + "0" + String(mi);
    String d2 = String(y) + "-" + String(m) + "-" +String(d) + " " + String(h) + ":" + String(mi);
  //u8g.setFont(font);
  u8g.firstPage();  
  do {
    u8g.drawStr( 10, 10, "Current Time");
    u8g.drawStr( 20, 40, change.c_str());
    u8g.drawStr( 20, 55, skip.c_str());
    u8g.drawStr( 10, pos, ">");
    if(mi < 10){     
      u8g.drawStr( 10, 25, d1.c_str() );
    }
    else{
      u8g.drawStr( 10, 25, d2.c_str() );
    }
  } while( u8g.nextPage() );
}
//Clock Selection menu
void setClockSelectText(int pos)
{
  u8g.setFont(u8g_font_6x13);
  u8g.firstPage();  
  do {
   u8g.drawStr( 10, pos, ">");
   u8g.drawStr( 10, 10, "Current Time");
   u8g.drawStr( 10, 25, "2-4-2019 17:12");
   u8g.drawStr( 20, 40, "Change");
   u8g.drawStr( 20, 55, "Skip");
    
  } while( u8g.nextPage() );
  delay(100);
}

//increase or decrease the seconds of sample rate based on arrow press and set text
void changeRate(boolean direction)
{
 switch(direction)
 {
 case 1:
    sample_rate = sample_rate + 60;
    if (sample_rate > 3540)
    {
      sample_rate = 120;
      }
    break;
 case 0:
    if(sample_rate > 120)
    {
      sample_rate = sample_rate - 60;
    }
    break;
  }
  delay(200);
}

//move the arrow on the screen and set the menu text accordingly
void moveArrow(boolean direction)
{
  //1 is up
  //0 is down
  switch(direction){
  case 1:    //up
      timerPos = 40; 
  
    break;
  case 0:    //down

      timerPos = 55;

    break;
   }
}

//set the text on the menu with arrow in the correct place
void setMenuText(int pos)
{
  u8g.setFont(u8g_font_7x14);
  u8g.firstPage();  
  do {
    //u8g.drawStr( 0, 15, "Main Menu");
    u8g.drawStr( 0, pos, ">");
    u8g.drawStr( 10, 15, "Start Log");
    u8g.drawStr( 10, 35, "Sample Rate");
    u8g.drawStr( 10, 55, "Set Time");
    
  } while( u8g.nextPage() );
  checkBLEreceiver();
  delay(100);
}

//log button pressed during log and returns to the menu screen
void stopLog()
{
  log_close();
  u8g.firstPage();  
  do {
    u8g.drawStr( 0, 15, "Logging Stopped");
  } while( u8g.nextPage() );
  delay(500);
  setMenuText(15);
  badBlood == 0;
  noTone(ALARM);
  timeStep = 0;                             // datapoint counting restarts from beginning
}


/**** Function for push-button handling ****/
boolean handle_select_button()                                 // source: http://www.instructables.com/id/Arduino-Button-Tutorial/
{
  boolean event;
  int button_now_pressed = !digitalRead(LOGBUTTONPIN); // pin low -> pressed
  event = button_now_pressed && !button_was_pressed;
  button_was_pressed = button_now_pressed;
  return event;
}

boolean handle_up_button()                                 // source: http://www.instructables.com/id/Arduino-Button-Tutorial/
{
  boolean event;
  int button_now_pressed = !digitalRead(UP); // pin low -> pressed
  event = button_now_pressed && !button_was_pressed;
  button_was_pressed = button_now_pressed;
  return event;
}

boolean handle_down_button()                                 // source: http://www.instructables.com/id/Arduino-Button-Tutorial/
{
  boolean event;
  int button_now_pressed = !digitalRead(DOWN); // pin low -> pressed
  event = button_now_pressed && !button_was_pressed;
  button_was_pressed = button_now_pressed;
  return event;
}

static void to_dec(uint16_t val, byte *buf, byte num)  //source: http://lallafa.de/blog/2012/11/build-an-arduino-based-temperature-logger/ //
{
  byte *ptr = buf + num - 1;
  for(byte i=0;i<num;i++) 
  {
    byte r = val % 10;
    val = val / 10;
    *(ptr--) = '0' + r;
  }
}

static void store_name()                       //source: http://lallafa.de/blog/2012/11/build-an-arduino-based-temperature-logger/ //
{
  read_clock();
  to_dec(month,fileName,2);
  to_dec(day,fileName+2,2);
  to_dec(year,fileName+4,2);
}

static byte log_find_name()                   //source: http://lallafa.de/blog/2012/11/build-an-arduino-based-temperature-logger/ //
{
  store_name();
  for(uint8_t i=0; i<676; i++) 
  {                // made a change here, put 65535 instead of 26, and made it uint16_t //
    fileName[6] = i/26 + 'a';                   // made a change here, put '1' instead of 'a' //
    fileName[7] = i%26 + 'a';
    if(!SD.exists((char *)fileName)) 
    {
      return 1;
    }
  }
  updateFile();
  return 0;
}

static byte log_open()
{
  // log already open!
  if(dataFile) 
  {
    return 1;
  }
  // try to find name
  if(!log_find_name()) 
  { 
    u8g.firstPage();  
    do {
     u8g.drawStr( 0, 15, "File Not Found");
    } while( u8g.nextPage() );
    return 2;
  }
  dataFile = SD.open((char *)fileName, FILE_WRITE);
  if(!dataFile) 
  {
    return 3;
  }
  read_clock();
  dataFile.println("&"+String(sample_rate)+"!");
  dataFile.println("*");
  log_is_open = 1;
  //String name1 = String(name);

  u8g.firstPage();  
  do {
    u8g.drawStr( 0, 15, "Wait for sensor ...");
    u8g.drawStr( 5, 35, (char *)fileName);
  } while( u8g.nextPage() );
  delay(5000);
  return 0;
}

static byte log_close()
{
  // log is not open!
  if(!dataFile) 
  {
    return 1;
  }
  read_clock();
  dataFile.println("%");
  delay(500);
  dataFile.close();
  // ok, log is closed
  log_is_open = 0;
  return 0;
}

/********** Initializing RTC DS3231 ************/
static void rtc_init()
{

  if (! rtc.begin()) 
  {
    while (1);
    OLEDscreen_sample(u8g_font_6x13, 10, 15, "No Real", 10, 35, "Time Clock", 10,55, "");
  }
  //if (! rtc.initialized()) 
  if (! rtc.initialized())
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    
  }
  //else
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

/************* Initializing SD Card **************/
static void sd_init()
{
  //Serial.println("Initializing Card");
  pinMode(CS_pin, OUTPUT);
  if (!SD.begin(CS_pin))
  { 
    flashRed(3);
    OLEDscreen_sample(u8g_font_6x13, 10, 15, "No SD Card", 10, 35, "Please Insert", 10,55, "then Reset");
    delay(3000);
  }
  //Serial.println("Card Ready");       // Notify user that the card is ready to be used
  delay(100);
}

/******** Function for reading time from RTC *********/
static void read_clock()
{
  now = rtc.now();
  year = now.year();
  month = now.month();
  day = now.day();
  hour = now.hour();
  minute = now.minute();
  second = now.second();
  unixtime = now.unixtime();
}

//void tcaselect(uint8_t i) {
//  if (i > 7) return;
 
//  Wire.beginTransmission(TCAADDR);
//  Wire.write(1 << i);
//  Wire.endTransmission();  
//}

void checkOpen(){

  if((digitalRead(BREECH) == HIGH)&&((timeOpen-timeClose) >=10)){
    tone(ALARM, 500);
    }
  else
    noTone(ALARM);
  }

void checkArrow(){
  boolean up = handle_up_button();
  boolean down = handle_down_button();
  
  if(up)
  {
    moveArrow(1);
    delay(100);
  }
  else if(down)
  {
    moveArrow(0);
    delay(100);
  }
  
  }

void checkBLEreceiver(){
  char rx_byte ;
  if (Serial.available()){
    rx_byte = Serial.read();
    Serial.println(rx_byte);
    if (rx_byte == 'a'){
      //Serial.println("Work!!!");
      //return 1;
      digitalWrite(BLUETOOTH, HIGH);  
      delay(1000);
      digitalWrite(BLUETOOTH, LOW);
      //check = 1;
      }
    }
  }

void checkCharger(){
  
  if(digitalRead(CHARGER)==HIGH)
    //Print to screen Charging
    flashRed(5);
  }

void setBrightness() {
  Wire.beginTransmission(0x3c);
  Wire.write(0x00);
  Wire.write(BRIGHTNESSREG);
  Wire.endTransmission();
  Wire.beginTransmission(0x3c);
  Wire.write(0x00);
  Wire.write(BRIGHTNESS);
  Wire.endTransmission();
}
void setOLED(){
  
  u8g.setContrast(0);
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }
  setBrightness();
  }

void sleepForTwoMinutes()
{
  for(int i=0;i<15;i++)
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

void wakeUp()
{
    //goto loop_start;
}

void fileDateTime(uint16_t* date, uint16_t* time) {
  //DateTime now = rtc.now();

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(year, month, day);

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(hour, minute, second);
}

void checkValue(){
  String inString ="";
  dataFile = SD.open("VALUE.TXT");
  if (dataFile) {
    //Serial.println("value.txt:");

    // read from the file until there's nothing else in it:
    while (dataFile.available()) {
      int test=dataFile.read();
      if (isDigit(test))
      {
        inString += (char)test;
        }
      if (test == '\n'){
      //Serial.print("Value:");
      sample_rate = inString.toInt();
      //Serial.println(sample_rate);
      
      inString ="";
      }
    }
    // close the file:
    dataFile.close();
  }
    }

void updateValue(){
  SD.remove("VALUE.TXT");
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  dataFile = SD.open("VALUE.TXT", FILE_WRITE);

  // if the file opened okay, write to it:
  if (dataFile) {

    dataFile.println(sample_rate);
    // close the file:
    dataFile.close();
  
      } 
    }
void checkFile(){
  String inString ="";
  dataFile = SD.open("NAME.TXT");
  if (dataFile) {
    //Serial.println("value.txt:");

    // read from the file until there's nothing else in it:
    while (dataFile.available()) {
      char test=dataFile.read();
      inString += (char)test;
      if (test == '\n'){
      //Serial.print("Value:");
      inString.toCharArray((char*)fileName, 13);
      //Serial.println(sample_rate);
      
      inString ="";
      }
    }
    // close the file:
    dataFile.close();
  }
    }
void updateFile(){
  SD.remove("NAME.TXT");
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  dataFile = SD.open("NAME.TXT", FILE_WRITE);

  // if the file opened okay, write to it:
  if (dataFile) {

    dataFile.println((char *)fileName);
    // close the file:
    dataFile.close();
  
      } 
    }
void secondsToHMS( const uint32_t seconds, uint16_t &h, uint8_t &m, uint8_t &s )
{
    uint32_t t = seconds;

    s = t % 60;

    t = (t - s)/60;
    m = t % 60;

    t = (t - m)/60;
    h = t;
}

void exitSample(){
  u8g.firstPage();  
    do {
  u8g.drawStr( 0, 15, "Exit Sample Rate");
    } while( u8g.nextPage() );
    delay(500);
  }
void exitTimer(){
  u8g.firstPage();  
    do {
  u8g.drawStr( 0, 15, "Exit Set Time");
    } while( u8g.nextPage() );
    delay(500);
  }

float checkBattery(){
  float battMax = 4200;        //Maximum battery
  float battMin = 3000;        //Minium battery
  //int batteryValue = 0;  //battery value
  //float battVol = 0;
  float battBar = 0;
  float battVol = 0;
  float battPercent = 0;
  float measuredvbat = analogRead(LOWBATTERY);
  //batteryValue = analogRead(LOWBATTERY);
  measuredvbat *=2;
  measuredvbat *= 3.3;
  measuredvbat /=1024;
  //battVol = map(measuredvbat,0,650,0,4200);
  battPercent = ((measuredvbat-battMin)*100L)/(battMax-battMin);
  battBar = (battPercent*20L)/100L ;
        if (battBar > 15){
          battBar = 20;
           }
        else if (battBar > 10 && battBar < 15){
           battBar = 15;
            }
        else if (battBar > 5 && battBar < 10){
           battBar = 10;
           }
        else if (battBar > 0 && battBar < 5){
            battBar = 5;
            }
         else {
            battBar =0;
           }
  //Serial.print("VBat: " ); Serial.println(measuredvbat);
  return battBar;
  }
void batteryLevel(){
  float batteryBar = checkBattery();
  String batteryIndicator;
  if (batteryBar=20) {
    batteryIndicator="FULL";
  }
  else if (batteryBar=10) {
    batteryIndicator="MEDIUM";
  }
  else if (batteryBar=15) {
    batteryIndicator="HIGH";
  }
  else
    batteryIndicator="LOW";
    
  u8g.setFont(u8g_font_6x13);
  u8g.firstPage();  
  do {
   u8g.drawStr( 10, 15, "Main Battery Level");
   u8g.drawStr( 50, 32, batteryIndicator.c_str());
   u8g.drawFrame(50,40,24,10);
   u8g.drawBox(52,42,batteryBar,6);
   u8g.drawBox(74,43,2,4);
  } while( u8g.nextPage() );
  delay(3000);
  }

void flashGreen(const byte times)
  {
  // flash LED
  for (int i = 0; i < times; i++)
    {
    digitalWrite (LEDSELECT, HIGH);
    delay (20);
    digitalWrite (LEDSELECT, LOW);
    delay (300);
    }
  }  // end of flashLED

 void flashRed(const byte times)
  {
  // flash LED
  for (int i = 0; i < times; i++)
    {
    digitalWrite (LEDONOFF, HIGH);
    delay (20);
    digitalWrite (LEDONOFF, LOW);
    delay (300);
    }
  }  // end of flashLED

boolean checkLidStt(){ 
  //check if the lid is open using hall effect
  if(digitalRead(BREECH) == HIGH)
     return 1;
  
  else
     return 0;
  
  }

void checkPayStt(float tempC){
 if(tempC > MAXTEMP || tempC < MINTEMP)
        {
          badBlood = 1;
        }
}


void clearOLED(){
    u8g.firstPage();  
    do {
    } while( u8g.nextPage() );
    delay(100);
}

void main_menu(int pos)
{
  u8g.setFont(u8g_font_7x14);
  u8g.firstPage();  
  do {
    //u8g.drawStr( 0, 15, "Main Menu");
    u8g.drawStr( 0, pos, ">");
    u8g.drawStr( 10, 15, "Start Log");
    u8g.drawStr( 10, 35, "Sample Rate");
    u8g.drawStr( 10, 55, "Set Time");
    
  } while( u8g.nextPage() );
  delay(100);
}

void OLEDscreen_sample(const u8g_fntpgm_uint8_t *font, int xline1, int yline1, String textLine1, int xline2, int yline2, String textLine2, int xline3, int yline3, String textLine3)
{
  u8g.setFont(font);
  u8g.firstPage();  
  do {
    u8g.drawStr( xline1, yline1, textLine1.c_str());
    u8g.drawStr( xline2, yline2, textLine2.c_str());
    u8g.drawStr( xline3, yline3, textLine3.c_str());
    
  } while( u8g.nextPage() );
  delay(100);
}


void drawLogo()
{
  u8g.firstPage();
  do{
   u8g.drawBitmapP( 0, 0, 16, 64, logo);
   } while(u8g.nextPage());
   delay(1000);
}
void wakeUpBLE(){
  }

void wakeUpLid(){
  }
boolean checkWireless(){
    while (zigBee.available()>0){
    //Read the incoming byte
    incomingByte = zigBee.read();
    //Serial.print("Receiver String:");
    //Serial.println(incomingByte);
    //Start the message when the '<' symbol is received
    if(incomingByte == '<')
    {
     started = true;
     indexStr = 0;
     msg[indexStr] = '\0'; // Throw away any incomplete packet
   }
   //End the message when the '>' symbol is received
   else if(incomingByte == '>')
   {
     ended = true;
     break; // Done reading - exit from while loop!
   }
   //Read the message!
   else
   {
     if(indexStr < 10) // Make sure there is room
     {
       msg[indexStr] = incomingByte; // Add char to array
       indexStr++;
       msg[indexStr] = '\0'; // Add NULL to end
     }
   }
   //if(started && ended)
   //return 1;
   //else 
   //return 0;
 }
}

void menu(void){
  checkPage();
  switch(page){
    case 0: checkArrow(); break;
    case 1: logging_menu(); break;
    case 2: sample_rate_menu(); break;
    case 3: set_time_menu(); break;
    }
  
  }


void checkPage(){
    
  }

void logging_menu(){
  //Start logging temperatures
    //open a file, loop through until log button is pressed again
     log_open();
     int checkLogRate = 0;
     badBlood = 0;
     flashGreen(3);
     loop_start:
     
     checkWireless();
     checkBLEreceiver();
     //Serial.print("Test: ");
     //Serial.println(started);
     //if log_open is successful and a file exists take data, otherwise there is no point in datalogging.
     if(digitalRead(LOGBUTTONPIN)==LOW)               // if '0' was received switch off data logging
        { 
          stopLog();
          //goto initializing;  
          //Serial.println(raising_edge_2);
          return;
        }
     if(started && ended)
     {
      if (dataFile)
      {
        if (log_is_open){
        flashGreen(1);
        read_clock();
        //LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        //sleepForTwoMinutes();
        //detachInterrupt(0); 
        if(strlen(msg) < 4)
        msg[0] = {'0'};
        String temp = String(msg);
        float tempC = temp.toFloat();
        //tempC = SHT2x.GetTemperature();
        checkPayStt(tempC);
 
        float battBar = checkBattery();
        boolean breech = checkLidStt();
       
        //string that gets saved in SD card and printed to bluetooth
        String printString = '~'+String(timeStep)+','+String(tempC)+','+String(breech)+','+String(badBlood)+','+ String(unixtime)+','+'@';
        //check on the sampling rate to save data
        
          //String format: ~timeStep,tempC,breech,bad@ 
          //breech = 0 open, 1 closed
          //badBlood = 0 good, 1 bad
          //write to sd card
          dataFile.println(printString);
          //write to bluetooth
          Serial.println(printString);
          //reset the count for sample rate
        
        delay(45);
        //Every 10 datapoints, close the datafile so that it saves the current file, then reopen it and continue writing. This will prevent empty files from occuring.
        if(timeStep % 10==0)
        {
            dataFile.close();
            dataFile = SD.open((char *)fileName, FILE_WRITE);
        }
        
        String date1 = String(year) + "-" + String(month) + "-" +String(day) + " " + String(hour) + ":0" + String(minute);
        String date2 = String(year) + "-" + String(month) + "-" +String(day) + " " + String(hour) + ":" + String(minute);
        u8g.setFont(u8g_font_7x14);
        u8g.firstPage();  
          do {
        if(minute < 10)
        {
            u8g.drawStr( 0, 15, date1.c_str() );
        }
        else
        {
            u8g.drawStr( 0, 15, date2.c_str() );
        }
        String temperature = "TEMP:" + String(tempC) +"C";
            u8g.drawStr( 0, 35, temperature.c_str() );
            //u8g.drawStr( 90, 55, percent.c_str() );
            u8g.drawFrame(50,80,24,10);
            u8g.drawBox(52,82,battBar,6);
            //u8g.drawBox(101,48,20,6);
            u8g.drawBox(74,83,2,4);
        
        if(digitalRead(BREECH) == HIGH)
        {
          timeOpen = hour*3600L + minute *60L + second;
            u8g.drawStr( 0, 55, "OPEN" );
        }
        if(digitalRead(BREECH) == LOW){
          timeClose = hour*3600L + minute *60L + second;
            u8g.drawStr( 0, 55, "CLOSE" );
          }

        checkOpen();
        if(badBlood == 1)
        {
          u8g.drawStr( 50, 55, "BAD" );
        }
        if(badBlood == 0)
        {
          u8g.drawStr( 50, 55, "GOOD" );
        }
        
         } while( u8g.nextPage() );
        timeStep += 10;
        //Serial.print("Check Time Step:");
        //Serial.println(timeStep);
       //Serial.print("Check Log Rate:");
        //Serial.println(checkLogRate);
        //delay(checkLogRate*1000);                            // Wait time
        delay(500);
        boolean raising_edge_2 =  handle_select_button();         // a dummy variable to observe whether the user wants to stop logging or not    
        indexStr = 0;
        msg[indexStr] = '\0';
        started = false;
        ended = false;
        
        goto loop_start;  
        //LowPower.sleep(5000); 
      }  
    }  
  }
  else goto loop_start; 
  }
void sample_rate_menu(){
  //boolean select_pressed =  handle_select_button();  
  //String sampleRate = String(sample_rate);
  //Serial.println(sample_rate);
  uint16_t hours;
  uint8_t minutes,seconds;
  secondsToHMS(sample_rate, hours, minutes, seconds);
  String hoursC = String(hours);
  String minutesC = String(minutes);
  String secondsC = String(seconds);
  //if(seconds<60){
    u8g.setFont(u8g_font_6x13);
    u8g.firstPage();  
      do {  
      u8g.drawStr( 0, 15, "Sample Rate: " );
      u8g.drawStr( 75, 15, minutesC.c_str());
      u8g.drawStr( 90, 15, "Minute" );
      u8g.drawStr( 0, 35, "Up/Down to Adjust");
      u8g.drawStr( 0, 55, "Select to Accept");
      } while( u8g.nextPage() );
      //Poll for the analog button press to change the value
      //state = analogRead(A3);
      boolean up = handle_up_button();
      boolean down = handle_down_button();
      boolean selectPressed =  handle_select_button();
      if(up)
      {
        changeRate(1);
      }
      else if(down)
      {
        changeRate(0);
      }
      //delay(200);
      //pressed the select button
      else if(selectPressed)
      {
        read_clock();
        updateValue();
        exitSample();        
        refresh_rate = sample_rate * 1000;
        //menuPos = 15;
        //setMenuText(15);
        state = STATE_TIME;
        //return;
        //goto initializing;
      }
    }
void set_time_menu(){
    //Select button pressed to change the time
    boolean selectPressed =  handle_select_button();
     //set RTC
    //get the current time as a starting point

    u8g.setFont(u8g_font_6x13);
    switch(timer_state){
    case 1:
    //show time on screen
      setClockMenuText(year, month, day, hour, minute, "Change", "Skip", timerPos);
      checkArrow();
      if(selectPressed && timerPos == 40){
        timer_state=2;
      }
      if(selectPressed && timerPos == 55){
        rtc.adjust(DateTime(year, month, day, hour, minute, 0));
        exitTimer();
        state = STATE_INIT;
        }
      
    break;
    
    case 2:
    //show time on screen
      setClockMenuText(year, month, day, hour, minute, "Changing Year", "", 40);
      year = setUnit(year, 2017, 9999);
      if(selectPressed){
        timer_state=3;
    }
    break;
    case 3:
      setClockMenuText(year, month, day, hour, minute, "Changing Month", "", 40);
      month = setUnit(month,2, 11);
      if(selectPressed){
        timer_state=4;
    }
    break;
    case 4:
      setClockMenuText(year, month, day, hour, minute, "Changing Day", "", 40);
      day = setUnit(day,2,30);
      if(selectPressed){
        timer_state=5;
    }
    break;
    case 5:
      setClockMenuText(year, month, day, hour, minute, "Changing Hour", "", 40);
      hour = setUnit(hour, 1,22);
      if(selectPressed){
        timer_state=6;
    }
    break;
    case 6:
      setClockMenuText(year, month, day, hour, minute, "Changing Minute", "", 40);
      minute = setUnit(minute, 1, 58);
      if(selectPressed){
        rtc.adjust(DateTime(year, month, day, hour, minute, 0));
        exitTimer();
        state = STATE_INIT;
   }
    break;            
  }
}
void set_init_menu(){
  timer_state = 1;
  u8g.setFont(u8g_font_6x13);
  boolean selectPressed =  handle_select_button();
  u8g.firstPage();  
  do {
    u8g.drawStr( 10, 15, "Please turn on");
    u8g.drawStr( 10, 35, "Wireless Sensor");
 } while( u8g.nextPage() );
  if(selectPressed){
    state = STATE_BLE;
  }
}

void download_BLE(){
  u8g.setFont(u8g_font_6x13);
  boolean selectPressed =  handle_select_button();
  u8g.firstPage();  
  do {
    u8g.drawStr( 10, 15, "Ready to download");
    u8g.drawStr( 10, 35, "data to phone");
 } while( u8g.nextPage() );
  if(selectPressed){
    state = STATE_WAKEUP_LID;
  }
}


void wakeup_lid(){
  u8g.setFont(u8g_font_6x13);
  boolean selectPressed =  handle_select_button();
  u8g.firstPage();  
  do {
    u8g.drawStr( 10, 15, "Please close");
    u8g.drawStr( 10, 35, "the lid");
 } while( u8g.nextPage() );
  if(selectPressed){
    state = STATE_SAMPLE;
  }
}

void wakeup_dur(){
  u8g.setFont(u8g_font_6x13);
  boolean selectPressed =  handle_select_button();
  u8g.firstPage();  
  do {
    u8g.drawStr( 10, 15, "Please download data");
    u8g.drawStr( 10, 35, "before Turn off");
 } while( u8g.nextPage() );
  if(selectPressed){
    state = STATE_SAMPLE;
  }
}
  
void loop() 
{
  //setBrightness();
  //read_clock();
  
  //checkArrow();
  //checkCharger(); //Optional
  //Receive the data from BLE phone
  
  //Internal starting point
  //initializing:
  
  switch (state){
  case STATE_SAMPLE:
      sample_rate_menu();
      //Serial.println("State: Sample Rate");
      //else
      //state = STATE_TIME;
      break;
  case STATE_TIME:
      //Serial.println("State: Timer");
      set_time_menu();
      break;
  case STATE_INIT:
      //Serial.println("State: Initial");
      set_init_menu();
      break;
  case STATE_LOG:
      Serial.println("State: Logging");
      //logging_menu();
      break;
  case STATE_BLE:
      download_BLE();
      break;
  case STATE_WAKEUP_LID:
      wakeup_lid();
      break;
  }
  /*
  // check whether the log button has been pressed and determine which part of the menu is being accessed
  if(select_pressed && menuPos == 15)
  {
    
   
 }
  //Menu postiiton set sample rate
  else if(select_pressed && menuPos == 35)
  {
    
  }
  //menu position pointing to set clock
  else if(select_pressed && menuPos == 55)
  {
 
    }*/
  }
