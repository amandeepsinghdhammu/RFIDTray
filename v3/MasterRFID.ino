#define DEBUG
// LIBERARIES
#include <SPI.h>
#include <MFRC522.h> //https://github.com/miguelbalboa/rfid
#include <printf.h>
#include <Wire.h>

/**
   MFRC522 configuration for Two RFID readers
   Typical pin layout used:
   ----------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino
               Reader/PCD   Uno/101       Mega      Nano v3
   Signal      Pin          Pin           Pin       Pin
   ----------------------------------------------------------
   RST/Reset   RST          8             8         8
   SPI SS      SDA(SS)      2             53        D2
   SPI MOSI    MOSI         11 / ICSP-4   51        D11
   SPI MISO    MISO         12 / ICSP-1   50        D12
   SPI SCK     SCK          13 / ICSP-3   52        D13
*/

const byte numReaders = 2;    // Two RFID readers
const byte ssPins[] = {2, 3}; // SS pins for those readers
const byte resetPin = 8;

MFRC522 mfrc522[numReaders]; // Initialization

// Global variables to store values for RFID readers for the program
bool rfid_tag_present_prev = false;
bool rfid_tag_present = false;

bool rfid_tag_present_prev2 = false;
bool rfid_tag_present2 = false;

int _rfid_error_counter = 0;
bool _tag_found = false;

int currentReadingReader1 = 0;
int currentReadingReader2 = 0;

//LED signals for different for reading RFID process
int blueLed1 = 6;  // Card scanning
int blueLed2 = 7; // Card Reading

String readRFID = "";
String readRFID1 = "stop";
String readRFID2 = "stop";
  
//Initialization
void setup(){

#ifdef DEBUG
  // Initialise  serial communications channel with PC
  Serial.begin(9600);
  printf_begin();
  Serial.println(F("Serial communication started"));
#endif

  Wire.begin();
  SPI.begin(); // Init SPI bus
  
  // Set digital pins as output
  pinMode(blueLed1, OUTPUT);
  pinMode(blueLed2, OUTPUT);

  // Initialize reader
  // Note that SPI pins on the reader must always be connected to certain
  // Arduino Pins (on an Uno, MOSI=> pin11, MISO=> pin12, SCK=>pin13
  // The Slave Select(SS) pin and reset pin can be assigned to any pin
  // Initialize MFRC522 Hardware

  for (uint8_t i = 0; i < numReaders; i++){
    mfrc522[i].PCD_Init(ssPins[i], resetPin);
    Serial.println(F("Reader #"));
    Serial.println(F(" initialised on pin "));
    Serial.println(String(ssPins[i]));
    Serial.println(F(". Antenna strength: "));
    Serial.println(mfrc522[i].PCD_GetAntennaGain());
    Serial.println(F(". Version : "));
    mfrc522[i].PCD_DumpVersionToSerial();
  }

  Serial.print(F("--- END STUFF ---"));
}

void loop(){
  
  rfid_tag_present_prev = rfid_tag_present;
  rfid_tag_present_prev2 = rfid_tag_present2;

  _rfid_error_counter += 1;
  if (_rfid_error_counter > 2){
    _tag_found = false;
  }

  // Detect Tag without looking for collisions
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);
  
  for (uint8_t i = 0; i < numReaders; i++){
    mfrc522[i].PCD_Init();
    // Reset baud rates
    mfrc522[i].PCD_WriteRegister(mfrc522[i].TxModeReg, 0x00);
    mfrc522[i].PCD_WriteRegister(mfrc522[i].RxModeReg, 0x00);
    // Reset ModWidthReg
    mfrc522[i].PCD_WriteRegister(mfrc522[i].ModWidthReg, 0x26);

    MFRC522::StatusCode result = mfrc522[i].PICC_RequestA(bufferATQA, &bufferSize);

    if (result == mfrc522[i].STATUS_OK){
      if (!mfrc522[i].PICC_ReadCardSerial()){ //Since a PICC placed get Serial and continue
        return;
      }
      _rfid_error_counter = 0;
      

      readRFID = dump_byte_array(mfrc522[i].uid.uidByte, mfrc522[i].uid.size);
      
      if(i+1 == 1){
        readRFID1 = readRFID;
        rfid_tag_present = true;
        currentReadingReader1 = i+1; //not 0 as 0 is our default dummy parameter
      }
      if(i+1 == 2){
        readRFID2 = readRFID;
        rfid_tag_present2 = true;
        currentReadingReader2 = i+1; //not 0 as 0 is our default dummy parameter
      }
    } else {
      if(i+1 == 2){
        rfid_tag_present2 = false;
      }
      if(i+1 == 1){
        rfid_tag_present = false;
      }
    }
  } //MFRC522 for loop end

  // Tag is present on RFID readers and fire up leds
  if ((rfid_tag_present && rfid_tag_present_prev) || (rfid_tag_present2 && rfid_tag_present_prev2)){
    
    if(readRFID1 != "stop") {
      ledOnOff("Reading", currentReadingReader1);
    }
    if(readRFID2 != "stop") {
      ledOnOff("Reading", currentReadingReader2);
    }
  }

  // rising edge
  if ((rfid_tag_present && !rfid_tag_present_prev) || (rfid_tag_present2 && !rfid_tag_present_prev2)){
    Serial.println(F("Card UID: "));
    sendDataToNrf(readRFID1, readRFID2);
  }

  // If tag is picked up from RFID reader
  if ((!rfid_tag_present && rfid_tag_present_prev)) { //tag1 gone
    Serial.println("Tag1 gone");
    readRFID1 = "stop";
    ledOnOff("Block", currentReadingReader1);
    sendDataToNrf(readRFID1, readRFID2);
  }

  if ((!rfid_tag_present2 && rfid_tag_present_prev2)) { //tag1 gone
    Serial.println("Tag2 gone");
    readRFID2 = "stop";
    ledOnOff("Block", currentReadingReader2);
    sendDataToNrf(readRFID1, readRFID2);
  }
  delay(100);
}

// To handle the RGB leds senerios
void ledOnOff(String status, int reader){
  if(reader == 1) {
    if(status == "Reading") {
      digitalWrite(blueLed1, HIGH);
    } else {
      digitalWrite(blueLed1, LOW);
    }  
  } else {
    if(status == "Reading") {
      digitalWrite(blueLed2, HIGH);
    } else {
      digitalWrite(blueLed2, LOW);
    }
  }
}

// Send data to NRF module through Serial communication to other arduino using TX/RX pins
void sendDataToNrf(String rfid1, String rfid2){

  Serial.println("======TRANSMITTING TO SLAVE ========");
  Serial.println("===== rfids start========");
  Serial.println("rfid1=" + rfid1);
  Serial.println("rfid2=" + rfid2);
  Serial.println("===== rfids end ========");
  Wire.beginTransmission(8); // transmit to device #8

  Wire.write(rfid1.c_str());
  Wire.write("-");
  Wire.write(rfid2.c_str());
  // Wire.write("-");
  // Wire.write(employeeId.c_str());
  Wire.endTransmission();
  Serial.println("======END TRANSMISSION TO SLAVE ========");
}

// To convert RFID hexadecimal to string
String dump_byte_array(byte *buffer, byte bufferSize){
  //   String s;
  unsigned long uiddec = 0;
  for (byte m = (bufferSize > 4 ? (bufferSize - 4) : 0); m < bufferSize; m++) {
    unsigned long p = 1;
    for (int k = 0; k < bufferSize - m - 1; k++){
      p = p * 256;
    }
    uiddec += p * buffer[m];
  }
  return String(uiddec);
}
