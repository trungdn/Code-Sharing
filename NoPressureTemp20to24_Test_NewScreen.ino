/*
* MaxQ Research Active Logger
* Purpose: This design is intended to be installed in MaxQ boxes for temperature controlled shipping containers.
* Hardware will be based around the ATMega328p processor with Arduino Bootloader, the SHT22 temperature and humidity sensor, a 20x4 LCD screen, Real time clock, Hall Effect sensor, microSD card, and BLE module (bluefruit).
* This code controls the processor which will have a front menu to select the various settings or start a log. Inside of a log, the data is taken from the sensors, printed on the LCD screen, saved to the SD card, and sent across BLE to a mobile app.
 */

//Libraries
#include <SD.h>
#include <RTClib.h>
#include <SHT2x.h> //temp sensor library
#include <Wire.h>                    
//#include <LiquidCrystal_I2C.h>
#include "U8glib.h"
//#include "SPI.h"
//#include "Adafruit_BMP280.h" //only needed in pressure programs.

//BLE Libs
#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

//Software Serial
/*
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);

*/
//Hardware Serial
//Adafruit_BluefruitLE_UART ble(Serial1, BLUEFRUIT_UART_MODE_PIN);

#define   TCAADDR 0x70
//#define   REDLEDPIN       A0          //SD Card status LED
#define   LOGBUTTONPIN    14           //Push button for selecting and logging on digital pin 5
#define   BREECH          15           //Digital pin to detect hall effect
#define   LEDSELECT       10           //LED when logging
#define   LEDONOFF        11           //LED when ON/OFF 
#define   MINTEMP         20          //minimum temperature for blood
#define   MAXTEMP         26          //maximum temperature for blood
#define   LOWBATTERY      A2          //Define battery percentage
#define   ALARM           A1          //Buzzer when open lid
#define   UP              12           //Up button
#define   DOWN            13           //Down button
#define   BLUETOOTH       2           //Bluetooth button


/* SLboat Add Device */
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);  // I2C 128x64(col2-col129) SH1106,Like HeiTec 1.3' I2C OLED 

boolean button_was_pressed;            // previous state of the log button
unsigned int refresh_rate = 1000;    // user input in milliseconds for time between samples      
unsigned long timeStep=0;       // Variable for time step increase and printing
float tempC;              //variable for temperature in Celcius
float humid;              //humidity as a percentage
boolean breech;               // value to show the state of the box (open or close)
int menuPos = 15;          // value to show where the menu arrow is to be pointed
int state = 0;            // value to show the analog button read value
int seconds = refresh_rate / 1000; //sample rate in seconds
//boolean lcdState;         // state of the backlight of the LCD screen
int timeOn;               //Start time of backlight
int badBlood;         //variable to show when blood has gone out of bounds
int batteryValue = 0;  //battery value
int timeOpen;         //Start time of open
int timeClose;     //Time close
/**** Defining RTC Variables ****/
DateTime        now;
static uint16_t year;
static uint8_t  month, day, hour, minute, second;

/**** Defining Datafile ****/
// offsets:    0123456789012
byte name[] = "yymmdd_x.txt";
File dataFile;
byte log_is_open;

/**** Allocation of I2C Address ****/
RTC_DS1307 rtc;                   // Allocating RTC address

/**** Pin configuration for a 16 chars 2 line LCD display ****/
//LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3, POSITIVE);

/**** SD Card Setup ****/
const int CS_pin = 4;  //We always need to set the CS Pin

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}


void setup() 
{
  Serial.begin(9600); 
  //BLE setup
  
  //I2C Screen
  
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

  //Initialize pins/variables/hardware/interrupts
  button_was_pressed = false;
  Wire.begin(); 
  pinMode(LEDSELECT, OUTPUT);
  pinMode(LEDONOFF, OUTPUT); 
  digitalWrite(LEDONOFF, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(5000);              // wait for 5 seconds
  digitalWrite(LEDONOFF, LOW);    // turn the LED off by making the voltage LOW 
  pinMode(LOGBUTTONPIN, INPUT);
  pinMode(BREECH, INPUT);
  digitalWrite(LOGBUTTONPIN, HIGH);         // pull-up enabled
  pinMode(UP, INPUT);
  digitalWrite(UP, HIGH);
  pinMode(DOWN, INPUT);
  digitalWrite(DOWN, HIGH);
  //pinMode(BACKLIGHT, INPUT);
  //digitalWrite(BACKLIGHT, HIGH);
  pinMode(BLUETOOTH, INPUT);
  digitalWrite(BLUETOOTH, HIGH);
  //pinMode(A2, OUTPUT);
  //digitalWrite(A2, LOW);
  //lcdState = false;
  //timeOn = 0;
  timeOpen =0;
  timeClose =0;
  //attachInterrupt(digitalPinToInterrupt(3),backLightOn,FALLING);
  attachInterrupt(digitalPinToInterrupt(2),bluetoothISR,FALLING);
  //lcd.begin(20,4);
  //lcd.noBacklight();// initialize the lcd for 16 chars 2 lines, turn off backlight
  rtc_init();                             // calling RTC initialization function
  sd_init();                              // calling SD card initialization function
  //tcaselect(3);
}

void loop() 
{
 
  
  //tcaselect(3);
  //BLE controller
  // Check for user input
  /*
  char inputs[BUFSIZE+1];
  char outputs[BUFSIZE+1];

  if ( getUserInput(inputs, BUFSIZE) )
  {
    // Send characters to Bluefruit
    Serial.print("[Send] ");
    Serial.println(inputs);

    ble.print("AT+BLEUARTTX=");
    ble.println(inputs);

    // check response stastus
    if (! ble.waitForOK() ) {
      Serial.println(F("Failed to send?"));
    }
  }

  // Check for incoming characters from Bluefruit
  ble.println("AT+BLEUARTRX");
  ble.readline();
  if (strcmp(ble.buffer, "OK") == 0) {
    // no data
    return;
  }
  // Some data was found, its in the buffer
  Serial.print(F("[Recv] ")); Serial.println(ble.buffer);
  if (strcmp(ble.buffer,"on") == 0){
  digitalWrite(BLUETOOTH, HIGH);}
  if (strcmp(ble.buffer,"off") == 0){
  bluetoothISR(); }
  else {
  ble.waitForOK();}
  */
  //Serial.println(timeup);
  //define menu start
  //Poll continuously for analog button press or log button press (interrupt pins are already taken)
  //analog buttson move arrow, log button selects pointed value
  //state = analogRead(A3);

  //boolean check = digitalRead(LOGBUTTONPIN);
  Serial.println(menuPos);
  checkArrow();
  /*
  if(state > 800 && state < 880)
  {
    moveArrow(0);
  }
  else if(state > 900 && state < 980)
  {
    moveArrow(1);
  }
  */
  //Internal starting point (left over by previous intern that I haven't improved yet.)
  initializing:
  // handle button

  //Test Screen
  /*u8g.firstPage();  
  do {
    u8g.drawStr( 0, 15, "TEST");
    } while( u8g.nextPage() );
    */
  read_clock();
  //checkLight();
  boolean raising_edge = handle_button();       
  // check whether the log button has been pressed and determine which part of the menu is being accessed
  if(raising_edge && menuPos == 35)
  {

    
    
    //Start logging temperatures
    //open a file, loop through until log button is pressed again
     log_open();
     int checkLogRate = 0;
     badBlood = 0;
     //internal loop of checking sensors and saving data (left over by previous intern that I haven't improved yet.)
     loop_start:
     //Use function as a timer to turn off backlight.
     //checkLight(); 
     checkOpen();
     //if log_open is successful and a file exists take data, otherwise there is no point in datalogging.
     if(log_is_open)
     {
      if (dataFile)
      {
        read_clock();
        tempC = SHT2x.GetTemperature();
        humid = SHT2x.GetHumidity();
        digitalWrite(LEDSELECT, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(1000);              // wait for a second
        digitalWrite(LEDSELECT, LOW);    // turn the LED off by making the voltage LOW
        delay(1000); 
        //Test Sensor
        //Serial.println("TEMP:");
        //Serial.println(tempC);
        //Serial.println("HUMID:");
        //Serial.println(humid);
        batteryValue = analogRead(LOWBATTERY);
        batteryValue = map(batteryValue,0,1024,0,100);
        String percent = String(batteryValue) + "%";
        //check if the lid is open using hall effect
       // if(digitalRead(BREECH) == HIGH)
         // breech = 0;
       // else
        //  breech = 1;
        delay(100);
        if(tempC > MAXTEMP || tempC < MINTEMP)
        {
          badBlood = 1;
        }
        //string that gets saved in SD card and printed to bluetooth
        String printString = '~'+String(timeStep)+','+String(tempC)+','+String(breech)+','+String(badBlood)+'@';
        //check on the sampling rate to save data
        if(checkLogRate == seconds)
        {
          //String format: ~timeStep,tempC,breech,bad@
          //breech = 0 open, 1 closed
          //badBlood = 0 good, 1 bad
          //write to sd card
          dataFile.println(printString);
          //write to bluetooth
          Serial.println(printString);
          //reset the count for sample rate
          checkLogRate = 0;
        }
        delay(100);
        //Every 10 datapoints, close the datafile so that it saves the current file, then reopen it and continue writing. This will prevent empty files from occuring.
        if(timeStep % 10)
        {
            dataFile.close();
            dataFile = SD.open((char *)name, FILE_WRITE);
        }
        //lcd.clear();
        //lcd.setCursor(0,0);
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
        //lcd.print(String(year) + "-" + String(month) + "-" +String(day) + " " + String(hour) + ":" + String(minute));
        //lcd.setCursor(0,1);
        String temperature = "TEMP:" + String(tempC) +"C";
        //lcd.print("TEMP:" + String(tempC) + (char)223 +"C");
            u8g.drawStr( 0, 35, temperature.c_str() );
          
        //lcd.setCursor(0,2);
        //lcd.print("H:" + String(humid) + "%");
        //lcd.setCursor(16,0);
        //lcd.print("100%");
            u8g.drawStr( 90, 55, percent.c_str() );
        
        if(digitalRead(BREECH) == LOW)
        {
          timeOpen = hour*60*60 + minute *60 + second;
          //lcd.setCursor(0,3);
          //lcd.print("OPEN");
            u8g.drawStr( 0, 55, "OPEN" );
        }
        if(digitalRead(BREECH) == HIGH){
          timeClose = hour*60*60 + minute *60 + second;
          //lcd.setCursor(0,3);
          //lcd.print("CLOSE");
            u8g.drawStr( 0, 55, "CLOSE" );
          }
        if(badBlood == 1)
        {
          //lcd.setCursor(16,3);
          //lcd.print("BAD");
          u8g.drawStr( 50, 55, "BAD" );
        }
         } while( u8g.nextPage() );
        timeStep += 1;
        checkLogRate++;
        delay(1000);                            // Wait time
        boolean raising_edge_2 = handle_button();         // a dummy variable to observe whether the user wants to stop logging or not    
        if(raising_edge_2)               // if '0' was received switch off data logging
        { 
          stopLog();
          goto initializing;  
        }
        else goto loop_start;      
      }  
    }  
  }
  //Menu postiiton set sample rate
  else if(raising_edge && menuPos == 15)
  {
    String sampleRate = String(seconds);
    //set sample rate
    //lcd.clear();
    //lcd.setCursor(0,0);
    //lcd.print("Sample Rate: ");
    //lcd.setCursor(16, 0);
    //lcd.print(seconds);
    //lcd.setCursor(0,1);
    //lcd.print("Up/Down to Adjust");
    //lcd.setCursor(0,2);
    //lcd.print("Enter to Accept");
    u8g.setFont(u8g_font_6x13);
    
    u8g.firstPage();  
      do {  
      u8g.drawStr( 0, 15, "Sample Rate: " );
      u8g.drawStr( 90, 15, sampleRate.c_str() );
      u8g.drawStr( 0, 35, "Up/Down to Adjust");
      u8g.drawStr( 0, 55, "Enter to Accept");
      } while( u8g.nextPage() );
    //Loop until the button to accept has been pushed.
    while(true)
    {
      //Poll for the analog button press to change the value
      //state = analogRead(A3);
      if(digitalRead(UP)==LOW)
      {
        changeRate(1);
      }
      else if(digitalRead(DOWN)==LOW)
      {
        changeRate(0);
      }
      delay(200);
      //pressed the log button
      boolean refreshAccept = handle_button();
      if(refreshAccept)
      {
        refresh_rate = seconds * 1000;
        menuPos = 15;
        setMenuText(15);
        goto initializing;
      }
    }
  }
  //menu position pointing to set clock
  else if(raising_edge && menuPos == 55)
  {
    //set RTC
    //get the current time as a starting point
    read_clock();
    int newYear = year;
    int newMonth = month;
    int newDay = day;
    int newHour = hour;
    int newMin = minute;
    String newYear1 = "Year: " + String(newYear);
    String newMonth1 = "Month: " + String(newMonth);
    String newDay1 = "Day: " + String(newDay);
    String newHour1 = "Hour: " + String(newHour);
    String newMin1 = "Min: " + String(newMin);

    u8g.setFont(u8g_font_6x13);

    u8g.firstPage();  
      do {
    //show time on screen
    setClockMenuText(newYear, newMonth, newDay, newHour, newMin);
    //lcd.setCursor(0,2);
    //lcd.print("Year: " + String(newYear));
    u8g.drawStr( 0, 55, "Changing Year");
    //set clock through series of loops, could find a better way to do this.
    } while( u8g.nextPage() );
    while(true)
    {
      newYear = setUnit(newYear, 2017, 9999);
      u8g.firstPage();  
      do {
      //start with year, then move through date variables until the end
      setClockMenuText(newYear, newMonth, newDay, newHour, newMin);
      //lcd.print("Year: " + String(newYear));
      u8g.drawStr( 0, 55, "Changing Year");
      } while( u8g.nextPage() );
      boolean yearAccept = handle_button();
      if(yearAccept)
      {
        u8g.firstPage();  
          do {
        setClockMenuText(newYear, newMonth, newDay, newHour, newMin);
        //lcd.setCursor(0,2);
        //lcd.print("Month: " + String(newMonth));
        u8g.drawStr( 0, 55, "Changing Month");
        } while( u8g.nextPage() );

        while(true)
        {
          newMonth = setUnit(newMonth,2, 11);
          u8g.firstPage();  
            do {
          setClockMenuText(newYear, newMonth, newDay, newHour, newMin);
          //lcd.print("Month: " + String(newMonth));
          u8g.drawStr( 0, 55, "Changing Month");
          } while( u8g.nextPage() );
          boolean monthAccept = handle_button();
          if(monthAccept)
          {
            u8g.firstPage();  
              do {
            setClockMenuText(newYear, newMonth, newDay, newHour, newMin);
            //lcd.setCursor(0,2);
            //lcd.print("Day: " + String(newDay));
            u8g.drawStr( 0, 55, "Changing Day");
            } while( u8g.nextPage() );

            while(true)
            {
              newDay = setUnit(newDay,2,30);
              u8g.firstPage();  
                do {
              setClockMenuText(newYear, newMonth, newDay, newHour, newMin);
              //lcd.print("Day: " + String(newDay));
              u8g.drawStr( 0, 55, "Changing Day");
              } while( u8g.nextPage() );
              boolean dayAccept = handle_button();
              if(dayAccept)
              {
                u8g.firstPage();  
                  do {
                setClockMenuText(newYear, newMonth, newDay, newHour, newMin);
                //lcd.setCursor(0,2);
                //lcd.print("Hour: " + String(newHour));
                  u8g.drawStr( 0, 55, "Changing Hour");
                  } while( u8g.nextPage() );
                while(true)
                {
                  newHour = setUnit(newHour, 1,22);
                  u8g.firstPage();  
                    do {
                    setClockMenuText(newYear, newMonth, newDay, newHour, newMin);
                  //lcd.print("Hour: " + String(newHour));
                    u8g.drawStr( 0, 55, "Changing Hour");
                    } while( u8g.nextPage() );
                  boolean hourAccept = handle_button();
                  if(hourAccept)
                  {
                    u8g.firstPage();  
                      do {
                      setClockMenuText(newYear, newMonth, newDay, newHour, newMin);
                    //lcd.setCursor(0,2);
                    //lcd.print("Minute: " + String(newMin));
                    
                      u8g.drawStr( 0, 55, "Changing Minute");
                      } while( u8g.nextPage() );
                    while(true)
                    {
                      u8g.firstPage();  
                      do {
                        newMin = setUnit(newMin, 1, 58);
                        setClockMenuText(newYear, newMonth, newDay, newHour, newMin);
                      //lcd.print("Minute: " + String(newMin));
                        u8g.drawStr( 0, 55, "Changing Minute");
                        } while( u8g.nextPage() );
                      boolean minAccept = handle_button();
                      if(minAccept)
                      {
                        //set all of the date variables accordingly
                        rtc.adjust(DateTime(newYear, newMonth, newDay, newHour, newMin, 0));
                        menuPos = 15;
                        setMenuText(15);
                        goto initializing;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
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
   dataFile=SD.open((char *)name);
   if (dataFile)
   {
    delay(1000);
    //print $$ to notify the app that we are sending a file which should open a new graph and plot all this data.
    Serial.println("$$");
    //loop through every byte of data and printit.
    while(dataFile.available())
    {
      Serial.write(dataFile.read());
      //delay in microseconds to buffer output, can't do delay() because we are in an interupt
      delayMicroseconds(13000);
    }
    //print ## to show app that the file has ended and should now plot all the data received
    Serial.println("##");
    dataFile.close();
   }
   //reopen the file and continue logging real time if it was open before.
   if(reopen == true)
   {
     dataFile = SD.open((char *)name, FILE_WRITE);
   }
   delayMicroseconds(16382);
   delayMicroseconds(16382);
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
  //lcd.setCursor(0,2);
  return unit;
}

/*
//interrupt function to turn on the back light and record what time it came on to be checked later.
void backLightOn()
{
  lcdState = true;
  timeOn = hour * 60 * 60 + minute * 60 + second;
}

//function called periodically to see how long the screen has been on (if it is on) and turn it off after 10 seconds or turn it on if it needs to be turned on (didnt work inside interupt).
void checkLight()
{
  int timeNow = hour * 60 * 60 + minute * 60 + second;
  if((lcdState == true) && ((timeNow - timeOn) >= 10))
  {
    lcd.noBacklight();
    lcdState = false;
  }
  else if(lcdState == true)
  {
    lcd.backlight();
  }
}
*/

//clock to set the text when changing time based on currently set variables
void setClockMenuText(int y, int m, int d, int h, int mi)
{
  
    String d1 = String(y) + "-" + String(m) + "-" +String(d) + " " + String(h) + ":" + "0" + String(mi);
    String d2 = String(y) + "-" + String(m) + "-" +String(d) + " " + String(h) + ":" + String(mi);

    //lcd.clear();
    //lcd.setCursor(0,0);
    //lcd.print("Set Time");
    u8g.drawStr( 0, 15, "Set Time");
    //lcd.setCursor(0,1);
    if(mi < 10)
    {
      //lcd.print(String(y) + "-" + String(m) + "-" +String(d) + " " + String(h) + ":" + "0" + String(mi));
    u8g.drawStr( 0, 35, d1.c_str() );
    }
    else
    {
      //lcd.print(String(y) + "-" + String(m) + "-" +String(d) + " " + String(h) + ":" + String(mi));
    u8g.drawStr( 0, 35, d2.c_str() );
    }
   
}

//increase or decrease the seconds of sample rate based on arrow press and set text
void changeRate(boolean direction)
{
  if(direction == 1)
  {
    seconds = seconds + 1;
  }
  else if(direction == 0)
  {
    if(seconds > 1)
    {
      seconds = seconds - 1;
    }
  }
  String sampleRate1 = String(seconds);
  u8g.firstPage();  
  do {
  //lcd.clear();
  //lcd.setCursor(0,0);
  //lcd.print("Sample Rate: ");
  //lcd.setCursor(16,0);
  //lcd.print(seconds);
  //lcd.setCursor(0,1);
  //lcd.print("Up/Down to Adjust");
  //lcd.setCursor(0,2);
  //lcd.print("Enter to Accept");
  u8g.drawStr( 0, 15, "Sample Rate: " );
  u8g.drawStr( 90, 15, sampleRate1.c_str() );
  u8g.drawStr( 0, 35, "Up/Down to Adjust");
  u8g.drawStr( 0, 55, "Enter to Accept");
  } while( u8g.nextPage() );
}

//move the arrow on the screen and set the menu text accordingly
void moveArrow(boolean direction)
{
  //1 is up
  //0 is down
  if(direction == 1)//up
  {
    if(menuPos > 15)
    {
      menuPos = menuPos - 20;
      setMenuText(menuPos);  
    }
  }
  else if(direction == 0)//down
  {
    if(menuPos < 55)
    {
      menuPos = menuPos + 20;
      setMenuText(menuPos);
    }
  }
}

//set the text on the menu with arrow in the correct place
void setMenuText(int pos)
{
  u8g.setFont(u8g_font_7x14);
  //lcd.clear();
  //lcd.setCursor(0,0);
  //lcd.print("Main Menu");
  //lcd.setCursor(0,pos);
  //lcd.print((char)126);
  //lcd.setCursor(4,1);
  //lcd.print("Start Log");
  //lcd.setCursor(4,2);
  //lcd.print("Set Sample Rate");
  //lcd.setCursor(4,3);
  //lcd.print("Set Time");
  u8g.firstPage();  
  do {
    //u8g.drawStr( 0, 15, "Main Menu");
    u8g.drawStr( 0, pos, ">");
    u8g.drawStr( 10, 15, "Start Log");
    u8g.drawStr( 10, 35, "Sample Rate");
    u8g.drawStr( 10, 55, "Set Time");
    
  } while( u8g.nextPage() );
  
  delay(500);
}

//log button pressed during log and returns to the menu screen
void stopLog()
{
  log_close();
  //lcd.clear();
  //lcd.print("Logging Stopped");
  u8g.firstPage();  
  do {
    u8g.drawStr( 0, 15, "Logging Stopped");
  } while( u8g.nextPage() );
  delay(500);
  setMenuText(15);
  timeStep = 0;                             // datapoint counting restarts from beginning
}

/*
inline void led_red(char on)        // turns on a red LED when data is being written to file
{
  digitalWrite(REDLEDPIN, on ? HIGH : LOW);
}
*/

/**** Function for push-button handling ****/
boolean handle_button()                                 // source: http://www.instructables.com/id/Arduino-Button-Tutorial/
{
  boolean event;
  int button_now_pressed = !digitalRead(LOGBUTTONPIN); // pin low -> pressed
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
  to_dec(year,name,2);
  to_dec(month,name+2,2);
  to_dec(day,name+4,2);
}

static byte log_find_name()                   //source: http://lallafa.de/blog/2012/11/build-an-arduino-based-temperature-logger/ //
{
  store_name();
  for(int i=0; i<26; i++) 
  {                // made a change here, put 65535 instead of 26, and made it uint16_t //
    name[7] = 'a' + i;                    // made a change here, put '1' instead of 'a' //
    if(!SD.exists((char *)name)) 
    {
      return 1;
    }
  }
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
    //lcd.clear();
    //lcd.setCursor(0,0);
    //lcd.print("File Not Found");    
    u8g.firstPage();  
    do {
     u8g.drawStr( 0, 15, "File Not Found");
    } while( u8g.nextPage() );
    return 2;
  }
  dataFile = SD.open((char *)name, FILE_WRITE);
  if(!dataFile) 
  {
    return 3;
  }
  read_clock();
  dataFile.println("{"+String(month)+'/'+String(day)+'/'+String(year)+"}");
  dataFile.println("("+String(hour)+':'+String(minute)+':'+String(second)+")");
  dataFile.println("&"+String(seconds)+"@");
  dataFile.println("~");
  //led_red(1);
  log_is_open = 1;
  //lcd.clear();
  //lcd.setCursor(4,0);
  //lcd.print("Logging");
  //lcd.setCursor(2,1);
  //lcd.print((char *)name);
  //String name1 = String(name);

  u8g.firstPage();  
  do {
    u8g.drawStr( 10, 15, "Logging");
    u8g.drawStr( 5, 35, (char *)name);
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
  dataFile.println("<"+String(month)+'/'+String(day)+'/'+String(year)+">");
  dataFile.println("["+String(hour)+':'+String(minute)+':'+String(second)+"]");
  delay(500);
  dataFile.close();
  // ok, log is closed
  log_is_open = 0;
//  led_red(0);
  return 0;
}

/********** Initializing RTC DS3231 ************/
static void rtc_init()
{
  rtc.begin();
  delay(100);
  if (! rtc.begin()) 
  {
    while (1);
  }
  if (! rtc.isrunning()) 
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

/************* Initializing SD Card **************/
static void sd_init()
{
  //Serial.println("Initializing Card");
  pinMode(CS_pin, OUTPUT);
  if (!SD.begin(CS_pin))
  {
      //lcd.setCursor(0,0);
      //lcd.print("No Memory Card");
      u8g.firstPage();  
        do {
      u8g.drawStr( 10, 15, "No Memory Card");
  } while( u8g.nextPage() );
      return;
  }
  //Serial.println("Card Ready");       // Notify user that the card is ready to be used
  delay(100);
  setMenuText(15);
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
}

void tcaselect(uint8_t i) {
  if (i > 7) return;
 
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}

void checkOpen(){

  //Serial.print("Close Time");
  //Serial.println(timeClose);
  //Serial.print("Open Time");
  //Serial.println(timeOpen);
  if((digitalRead(BREECH) == LOW)&&((timeOpen-timeClose) >=10)){
    tone(ALARM, 500);
    }
  else
    noTone(ALARM);
  }

void checkArrow(){
  
  if(digitalRead(UP)==LOW)
  {
    moveArrow(1);
  }
  else if(digitalRead(DOWN)==LOW)
  {
    moveArrow(0);
  }
  
  }

bool getUserInput(char buffer[], uint8_t maxSize)
{
  // timeout in 100 milliseconds
  TimeoutTimer timeout(100);

  memset(buffer, 0, maxSize);
  while( (!Serial.available()) && !timeout.expired() ) { delay(1); }

  if ( timeout.expired() ) return false;

  delay(2);
  uint8_t count=0;
  do
  {
    count += Serial.readBytes(buffer+count, maxSize);
    delay(2);
  } while( (count < maxSize) && (Serial.available()) );

  return true;
}
