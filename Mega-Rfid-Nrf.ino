#define DEBUG

// LIBERARIES
#include <SPI.h>
#include <MFRC522.h>
#include <RF24.h>
#include <printf.h>

#define RST_PIN         5          // Configurable, see typical pin layout above
#define SS_PIN          53         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

bool rfid_tag_present_prev = false;
bool rfid_tag_present = false;
int _rfid_error_counter = 0;
bool _tag_found = false;

/**
   Configuraton for NRF24l01 sensor
*/
// create RF24 radio object using selected CE and CSN pins
RF24 radio(7, 8);

/**
   InitializationM
*/
void setup() {
  #ifdef DEBUG
    // Initialise  serial communications channel with PC
    Serial.begin(9600);
    printf_begin();
    Serial.println(F("Serial communication started"));
  #endif
  
  SPI.begin();      // Init SPI bus

  // Initialize reader
  // Note that SPI pins on the reader must always be connected to certain
  // Arduino Pins (on an Uno, MOSI=> pin11, MISO=> pin12, SCK=>pin13
  // The Slave Select(SS) pin and reset pin can be assigned to any pin
  mfrc522.PCD_Init(SS_PIN, RST_PIN);    // Initialize MFRC522 Hardware

  Serial.print(F("Reader #"));
  Serial.print(F(" initialised on pin "));
  Serial.print(String(SS_PIN));
  Serial.print(F(". Antenna strength: "));
  Serial.print(mfrc522.PCD_GetAntennaGain());
  Serial.print(F(". Version : "));
  mfrc522.PCD_DumpVersionToSerial();

  radio.begin();
  radio.setPALevel(RF24_PA_MIN);
  radio.setChannel(0x76);
  radio.openWritingPipe(0xF0F0F0F0E1LL);
  radio.enableDynamicPayloads();
  radio.setDataRate(RF24_250KBPS);
  radio.powerUp();  
  radio.printDetails();
  Serial.print(F("--- END STUFF ---"));
  
  mfrc522.PCD_Init();   // Init MFRC522
}

void loop() {
  rfid_tag_present_prev = rfid_tag_present;

  _rfid_error_counter += 1;
  if(_rfid_error_counter > 2){
    _tag_found = false;
  }

  // Detect Tag without looking for collisions
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);

  // Reset baud rates
  mfrc522.PCD_WriteRegister(mfrc522.TxModeReg, 0x00);
  mfrc522.PCD_WriteRegister(mfrc522.RxModeReg, 0x00);
  // Reset ModWidthReg
  mfrc522.PCD_WriteRegister(mfrc522.ModWidthReg, 0x26);

  MFRC522::StatusCode result = mfrc522.PICC_RequestA(bufferATQA, &bufferSize);

  if(result == mfrc522.STATUS_OK){
    if ( ! mfrc522.PICC_ReadCardSerial()) { //Since a PICC placed get Serial and continue   
      return;
    }
    _rfid_error_counter = 0;
    _tag_found = true;        
  }
  
  rfid_tag_present = _tag_found;
  
  // rising edge
  if (rfid_tag_present && !rfid_tag_present_prev){
    Serial.println("Tag found");
    String readRFID;
    readRFID = dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.print(F("Card UID: "));
    Serial.print(readRFID);
    Serial.print(sizeof(readRFID));
    Serial.println();
    sendData(readRFID);
  }
  
  // falling edge
  if (!rfid_tag_present && rfid_tag_present_prev){
    Serial.println("Tag gone");
    sendData("stop");
  }
  
}

void sendData(String data) {
  // Length (with one extra character for the null terminator)
  int str_len = data.length() + 1; 
   
  // Prepare the character array (the buffer) 
  char char_array[str_len];
   
  // Copy it over 
  data.toCharArray(char_array, str_len);
  radio.write(&char_array, sizeof(char_array));
  Serial.println("Data send");
  radio.powerDown();
  radio.powerUp();
}


String dump_byte_array(byte *buffer, byte bufferSize) {
  //   String s;
  unsigned long uiddec = 0;
  //    unsigned long temp;
  //char uid[8];
  for (byte m = (bufferSize > 4 ? (bufferSize - 4) : 0); m < bufferSize; m++) { //берем только последние 4 байта и переводим в десятичную систему
    unsigned long p = 1;
    for (int k = 0; k < bufferSize - m - 1; k++) {
      p = p * 256;
    }
    uiddec += p * buffer[m];
    //   s = s + (buffer[m] < 0x10 ? "0" : "");
    //   s = s + String(buffer[m], HEX);
  }
  //    s.toCharArray(uid, 8);
  return String(uiddec);
}
