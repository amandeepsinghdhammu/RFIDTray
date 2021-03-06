// master
/*
  #include <Wire.h>

  void setup() {
  Wire.begin(); // join i2c bus (address optional for master)
  }

  byte x = 0;

  void loop() {
  Wire.beginTransmission(8); // transmit to device #8
  Wire.write("x is ");        // sends five bytes
  Wire.write(x);              // sends one byte
  Wire.endTransmission();    // stop transmitting

  x++;
  delay(500);
  }
*/

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

// Global variables to store values for the program
bool rfid_tag_present_prev = false;
bool rfid_tag_present = false;
int _rfid_error_counter = 0;
bool _tag_found = false;

// Auth configuration
String EmployeeRFIDCode = "3802552960";

const byte employeeArrayCount = 4;
String employeeArray[employeeArrayCount] = {"117880691", "2897088462", "3802552960", "3779262765"};

String arrayIndex;

bool auth = false;
unsigned long lockTime = 10000;  // 10 sec
unsigned long authStartTime = 0; // When auth start
unsigned long currentTime = 0;   // To hold current millis

//LED signals for different for reading RFID process

int RedLED = 4;   // no authorize to scan
int GreenLED = 5; // ready to scan
int BlueLED = 6;  // Card already scanned

//Initialization

void setup()
{
#ifdef DEBUG
  // Initialise  serial communications channel with PC
  Serial.begin(9600);
  printf_begin();
  Serial.println(F("Serial communication started"));
#endif

  Wire.begin();

  SPI.begin(); // Init SPI bus

  // Set digital pins as output
  pinMode(RedLED, OUTPUT);
  pinMode(GreenLED, OUTPUT);
  pinMode(BlueLED, OUTPUT);

  // Initial red light on
  ledOnOff("block");

  // Initialize reader
  // Note that SPI pins on the reader must always be connected to certain
  // Arduino Pins (on an Uno, MOSI=> pin11, MISO=> pin12, SCK=>pin13
  // The Slave Select(SS) pin and reset pin can be assigned to any pin
  // Initialize MFRC522 Hardware

  for (uint8_t i = 0; i < numReaders; i++)
  {
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

void loop()
{

  currentTime = millis();

  if ((currentTime - authStartTime) >= lockTime && authStartTime != 0 && auth)
  {
    auth = false;
    ledOnOff("block"); // red light on
  }

  rfid_tag_present_prev = rfid_tag_present;

  _rfid_error_counter += 1;
  if (_rfid_error_counter > 2)
  {
    _tag_found = false;
  }

  // Detect Tag without looking for collisions
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);
  String readRFID = "";

  for (uint8_t i = 0; i < numReaders; i++)
  {
    mfrc522[i].PCD_Init();
    // Reset baud rates
    mfrc522[i].PCD_WriteRegister(mfrc522[i].TxModeReg, 0x00);
    mfrc522[i].PCD_WriteRegister(mfrc522[i].RxModeReg, 0x00);
    // Reset ModWidthReg
    mfrc522[i].PCD_WriteRegister(mfrc522[i].ModWidthReg, 0x26);

    MFRC522::StatusCode result = mfrc522[i].PICC_RequestA(bufferATQA, &bufferSize);

    if (result == mfrc522[i].STATUS_OK)
    {
      if (!mfrc522[i].PICC_ReadCardSerial())
      { //Since a PICC placed get Serial and continue
        return;
      }
      _rfid_error_counter = 0;
      _tag_found = true;

      readRFID = dump_byte_array(mfrc522[i].uid.uidByte, mfrc522[i].uid.size);
    }

    rfid_tag_present = _tag_found; // not emp tag

  } //MFRC522 for loop end

  // rising edge
  if (rfid_tag_present && !rfid_tag_present_prev)
  {
    Serial.println("Tag found");
    Serial.println(F("Card UID: "));
    Serial.println();
    Serial.println(sizeof(readRFID));
    Serial.println();

    // Check if RFID exist in employee array
    //arrayIndex = findIndexInArray(employeeArray, employeeArrayCount, readRFID);

    if (EmployeeRFIDCode == readRFID)
      //if (arrayIndex != readRFID)
    {
      Serial.println("=============Admin card found===================");
      // Found
      Serial.println(readRFID + "==" + EmployeeRFIDCode);
      Serial.println("Auth true");
      auth = true;
      ledOnOff("authorize");
      authStartTime = millis();
    }
    else
    {
      // Not Found
      Serial.println("=============In else===================");
      if (auth)
      {
        ledOnOff("authorize");
        Serial.println("=============Ready to send===================");
        authStartTime = millis();
        sendDataToNrf(readRFID, EmployeeRFIDCode);
      } else {
        ledOnOff("block");
      }
    }
  }

  // If a tag is present on RFID reader
  if (rfid_tag_present && rfid_tag_present_prev)
  {
    authStartTime = millis();
    if (auth)
    {
      Serial.println("====Tag on rfid and not emp tag then show blue===");
      if (EmployeeRFIDCode != readRFID) {
        ledOnOff("reading");
      }

    }
  }

  // If tag is picked up from RFID reader
  if (!rfid_tag_present && rfid_tag_present_prev)
  {
    Serial.println("Tag gone");
    if (auth)
    {      
      ledOnOff("authorize");
      authStartTime = millis();
      sendDataToNrf("stop", "stop");
    } else {
      ledOnOff("block");
    }

  }
}

// To handle the RGB leds senerios
void ledOnOff(String status)
{
  if (status == "authorize")
  { // authorize to scan card
    digitalWrite(GreenLED, HIGH);
    digitalWrite(RedLED, LOW);
    digitalWrite(BlueLED, LOW);
  }
  else if (status == "reading")
  { //reading tag
    digitalWrite(BlueLED, HIGH);
    digitalWrite(GreenLED, LOW);
    digitalWrite(RedLED, LOW);
  }
  else
  { //block [dont allow to scan]
    digitalWrite(BlueLED, LOW);
    digitalWrite(GreenLED, LOW);
    digitalWrite(RedLED, HIGH);
  }
}

// Send data to NRF module through Serial communication to other arduino using TX/RX pins
void sendDataToNrf(String rfid, String employeeId)
{

  Serial.println("======TRANSMITTING TO SLAVE ========");
  Serial.println(rfid);
  Wire.beginTransmission(8); // transmit to device #8

  Wire.write(rfid.c_str());
  Wire.write("-");
  Wire.write(employeeId.c_str());
  Wire.endTransmission();
  Serial.println("======END TRANSMISSION TO SLAVE ========");
}

// To convert RFID hexadecimal to string
String dump_byte_array(byte *buffer, byte bufferSize)
{
  //   String s;
  unsigned long uiddec = 0;
  for (byte m = (bufferSize > 4 ? (bufferSize - 4) : 0); m < bufferSize; m++)
  {
    unsigned long p = 1;
    for (int k = 0; k < bufferSize - m - 1; k++)
    {
      p = p * 256;
    }
    uiddec += p * buffer[m];
  }
  return String(uiddec);
}

// To find an element in an array(https://phanderson.com/C/find_idx.html)
String findIndexInArray(String a[], int num_elements, String value)
{

  int i;
  for (i = 0; i < num_elements; i++)
  {
    if (a[i] == value)
    {
      return (value); // it was found
    }
  }
  return (-1); // if it was not found
}
