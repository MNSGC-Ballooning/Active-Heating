/*
Joseph Habeck, March 2018

Summary: code to run heater module; reads in temperature from both analog and digital sensors checks if temperature is below user-defined 
critical LOW temperature, if so, turns on heater; heater remains on until temperature is above critical HIGH temp.;
temperature readings, heater operation status, and flight time is written to SD card. Note that the temperature reading used to 
compare to the critical temps. is read from the digital sensor. 

The circuit includes:
-- Arduino Uno
-- Sparkfun micro SD shield (chip select pin = 8)
-- OMRON G5V-2 relay switch (digital pin 4)
-- TMP analog temperature sensor (analog pin 0)
-- Dallas digital temperature sensor (digital pin 2)
-- (2) 9v battery power supply 
 */


#include <SPI.h>
#include <SD.h>

// Used for Dallas temp sensor:
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
//

int tPin1 = 0; // set analog pin 0 (analog temp sensor)
int tPin2 = 2; // set digital pin 2 (digital temp sensor)
int qSwitch = 4; // set digital pin 4 (relay)
int hold; // used for heater switch

float t1; // temperature read from analog sensor
float t2; // temperature read from digital sensor
float samp_freq=3000; // sampling frequency (ms)
float t_low = 283; // critical low temp [K] (heater turns ON if t2 < t_low)
float t_high = 289; // critical high temp [K] (heater turns OFF if t2 > t_high)

String data; // used for data logging
String heaterStatus; // used for data logging; will be "on" or "off" 
String filename = "tempLog.csv"; // file name that data wil be written to
const int CS = 8; // CS pin on SD card
File tempLog; // file that data is written to 

unsigned long t; // flight-time

void setup() {
  Serial.begin(9600); // open serial port
  sensors.begin(); // start up Dallas temp sensor library 
  
  // Initalize digital pins:
  pinMode(tPin2, INPUT);
  pinMode(qSwitch, OUTPUT);
  
  // Open serial port(s):
  while (!Serial){
    ; // wait for serial port to connect
  }

  Serial.print("Initializing SD card...");

  // Check if card is present/initalized: 
  if (!SD.begin(CS)){
  Serial.println("card initialization FAILED - something is wrong..."); // card not present or initialization failed
  while (1); // dont do anything more
  }
  
  Serial.println("card initialization PASSED... bon voyage!"); // initialization successful

  // Initialize file:
  tempLog = SD.open(filename, FILE_WRITE); // open file
  
  if (tempLog) {
    Serial.println( filename + " opened...");
    tempLog.println("Temperature 1 (analog) (K), Temperature 2 (digital) (K), Heater Status, Flight Time (hour:min:sec)"); // file heading
    tempLog.close();
    Serial.println("file initialized...start data logging!");
  }
  else {
    Serial.println("error opening file");
    return;
  }
}

void loop() {
  
  t = millis() / 1000; // set flight-time variable to Arduino internal clock
  
  ////////// Temperature monitoring ////////// 
  
  t1 = getAnalogTemp(tPin1); // analog temp in Kelvin
  sensors.requestTemperatures(); // request temp from digital temp sensor...
  t2 = sensors.getTempCByIndex(0); // digital temp in celcius
  t2 = t2 + 273.15; // digital temp in Kelvin

  ////////// Heater operation //////////

// "test-fire" heater for 3 minutes after Arduino clock has started; NOTE heater does not depend on temperature values during this time!
  if (t<(60*3)){
   digitalWrite(qSwitch, HIGH); 
   heaterStatus = "on";
  }
  
// compare digital temp. to critical temp.:  
  else{
    if (t2 < t_low) {
      hold = 1; // if temperature is below low critical temperature
    }
    if (t2 > t_high) {
      hold = 0; // if temperature is above high critical temperature
     }    

// turn heater on/off:
    if (hold==1){
    digitalWrite(qSwitch, HIGH); 
    heaterStatus = "on";
    }
   else {
    digitalWrite(qSwitch, LOW);
    heaterStatus = "off";
    }  
  }

////////// Datalogging //////////

data = "";
data += t1; // log value of analog temp.
data += ","; // log value of digital temp. 
data += t2;
data += ",";
data += heaterStatus; // log heater status (either "on" or "off")
data += ",";
data += flightTime(t); // log flight time; flightTime is a user-defined function

////////// Data Writing //////////

 tempLog = SD.open("tempLog.csv", FILE_WRITE); // open file

 if (tempLog) {
    //Serial.println("tempLog.csv opened..."); // file open successfully 
    tempLog.println(data);
    tempLog.close();
    Serial.println(data);
  }
  else {
    Serial.println("error opening file"); // file open failed
    return;
  }

 delay(samp_freq); // delay 1 second i.e. do all that every 1 second 

 }

///////// User-defined functions //////////

// Reads in temp. from analog sensor and converts it to Kelvin; written by: Joey Habeck
float getAnalogTemp(int pin) {
  float t = analogRead(pin) * .004882814; // analog temperature
  float t_F = (((t - .5) * 100) * 1.8) + 32; // convert temperature to Farenheit
  float t_K = (t_F + 459.67) * 5 / 9; // convert Farenheit to Kelvin
  return (t_K);
}

// Reads in time from Arduino clock (seconds) and converts it to hr:min:sec; written by: Simon Peterson 
String flightTime(unsigned long t) {
  String fTime = "";
  fTime += (String(t / 3600) + ":");
  t %= 3600;
  fTime += String(t / 600);
  t %= 600;
  fTime += (String(t / 60) + ":");
  t %= 60;
  fTime += (String(t / 10) + String(t % 10));
  return fTime;
}

