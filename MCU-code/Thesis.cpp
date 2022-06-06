#include <SPI.h>
#include <SD.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"


Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 10;
const int analogInPin = A0;
int detected_guesture = 0;
const int threshold = 450;
String logMsg, collectedData, temp;
int window_size = 3;
int sensorValue = 0;        
String last_known_guesture= "Nutral";
int last_guesture = 0;


// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}


void log_data(String data){
  digitalWrite(6,LOW);

  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(data);
    dataFile.close();
    // print to the serial port too:
    Serial.println(data);
  }
  digitalWrite(6,HIGH);
}

int slide_Window_detection(){
  logMsg = "";
  volatile int sensor_reading = 0;
  temp = "{";
  int sum = 0, avg = 0;
  int three_items[window_size] = {0};
  int memory_var = 0;
  for(int i =0; i <252; i++){
    sensor_reading = analogRead(analogInPin);
    three_items[(i%window_size)] = sensor_reading;
    if((i%window_size) == 0)
    {
      for(int j=0;j<window_size;j++)
      {
         sum += three_items[j];  
      }
      avg = sum/window_size;
      temp.concat(String(avg));
      temp.concat(", ");
      sum = 0;
    
      if(avg > threshold){
        memory_var++;
      }    
    }
    delay(2);
  }

  if (memory_var >= 65){
    logMsg.concat(String(threshold));
    logMsg.concat(String(2));
    logMsg.concat(temp);
    logMsg.concat("}");
    log_data(logMsg);
    return 2;//box 
  }
  else{
    logMsg.concat(String(threshold));
    logMsg.concat(String(1));
    logMsg.concat(temp);
    logMsg.concat("}");
    log_data(logMsg);
    return 1;
  }
}

void init(void)
{
  digitalWrite(6,LOW);
  Serial.begin(9600);
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");}

  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println("New session");
    dataFile.close();
    // print to the serial port too:
    Serial.println("New session");
  }
  digitalWrite(6,HIGH);
  
  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  /* Print Bluefruit information */
  ble.info();


  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activit
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set module to DATA mode
  ble.setMode(BLUEFRUIT_MODE_DATA);
}


void main(void)
{
  init();
  // Check for user input
  char n, inputs[BUFSIZE+1];
  detected_guesture = 0;
  sensorValue = analogRead(analogInPin);
  if ((sensorValue > threshold) && (last_guesture == 0)){
    ble.println("JUMP");
    detected_guesture = slide_Window_detection();
    last_guesture = detected_guesture;
  }
  else if (sensorValue < 80){
    last_guesture = 0;
  }

    if (detected_guesture == 1){
    last_known_guesture = "KIKI";
  }
  else if (detected_guesture == 2){
    last_known_guesture = "BOX";
  }


  if (detected_guesture != 0){
      ble.println(last_known_guesture);}
    delay(20);
}
