/*   ~ Simple Arduino - xBee Transmitter sketch ~

  Read an analog value from potentiometer, then convert it to PWM and finally send it through serial port to xBee.
  The xBee serial module will send it to another xBee (resiver) and an Arduino will turn on (fade) an LED.
  The sending message starts with '<' and closes with '>' symbol. 
  
  Dev: Michalis Vasilakis // Date:2/3/2016 // Info: www.ardumotive.com // Licence: CC BY-NC-SA                    */
#include <Wire.h> //SHT2x
#include <SHT3x.h> //SHT3x
//#include <SHT2x.h> //SHT2x
SHT3x Sensor; //SHT3x
#include "LowPower.h"
#define POWER 3
//Constants: 
//const int potPin = A0; //Pot at Arduino A0 pin 
//Variables:
//int value ; //Value from pot

void setup() {
  //Start the serial communication
  //Wire.begin(); //SHT2x
  Serial.begin(9600); //Baud rate must be the same as is on xBee module
  Sensor.Begin();//SHT3x
  //Serial.println("Hello");
  /*
  for (byte pin = 0; pin < 14; pin++)
    {
    pinMode (pin, OUTPUT);
    digitalWrite (pin, LOW);
    }
    */
  pinMode (A4, OUTPUT);
  pinMode (A5, OUTPUT);
  //pinMode (0, OUTPUT);
  //pinMode (1, OUTPUT);
  digitalWrite (A4, LOW);
  digitalWrite (A5, LOW);
  //digitalWrite (0, LOW);
  //digitalWrite (1, LOW);
  pinMode(POWER, OUTPUT);
  digitalWrite(POWER, LOW);

  pinMode(2, OUTPUT);
}


void loop() {
  Serial.println("Hi");
  //digitalWrite (A4, LOW);
  //digitalWrite (A5, LOW);
  powerOnPeripherals ();
  Serial.println("Power On");
  digitalWrite(2, HIGH);
  delay(1000);
  getTemp();
  
  
  powerOffPeripherals ();
  Serial.println("Power Off");
  digitalWrite(2, LOW);
  
  sleepForTwoMinutes();
  //delay(5000);
}

void sleepForTwoMinutes()
{
  for(int i=0;i<2;i++)
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
}

void powerOnPeripherals ()
  {
  //pinMode(POWER, OUTPUT);
  digitalWrite (POWER, HIGH);  // turn power ON
  delay (10); // give time to power up    
  } 

void powerOffPeripherals ()
  {
  //digitalWrite (A4, LOW);
  //digitalWrite (A5, LOW);
  digitalWrite (POWER, LOW);  // turn power OFF
  delay (10); // give time to power up    
  } 
void powerOnSHT(){
  //Serial.begin(9600);
  Sensor.UpdateData();
  }

void powerOffSHT(){
  TWCR &= ~(bit(TWEN) | bit(TWIE) | bit(TWEA));
  digitalWrite (A4, LOW);
  digitalWrite (A5, LOW);
  //Serial.end();
  delay(100);
  }

void getTemp(){
  powerOnSHT();
  Serial.print('<');  //Starting symbol
  Serial.print(Sensor.GetTemperature()); //SHT3x
  //Serial.println(SHT2x.GetTemperature()); //SHT2x
  Serial.println('>');//Ending symbol
  delay(555);
  powerOffSHT();
  }
