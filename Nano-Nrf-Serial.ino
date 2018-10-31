#define DEBUG

// LIBERARIES
#include <SPI.h>
#include <RF24.h> //http://maniacbug.github.com/RF24
#include <printf.h>

/**
   Configuraton for NRF24l01 sensor
*/
// create RF24 radio object using selected CE and CSN pins
RF24 radio(9, 10);

String incomingValue;
bool readStart = false;
int i = 0;
String employeeRFID = "";
String rfid = "";

/**
   Initialization
*/
void setup()
{
#ifdef DEBUG
    // Initialise  serial communications channel with PC
    Serial.begin(9600);
    printf_begin();
    Serial.println(F("Serial communication started"));
#endif

    SPI.begin(); // Init SPI bus

    // initialize the NRF2401 Module
    radio.begin();
    radio.setPALevel(RF24_PA_MIN);
    radio.setChannel(0x76);
    radio.openWritingPipe(0xF0F0F0F0E1LL);
    radio.enableDynamicPayloads();
    radio.powerUp();
    radio.setDataRate(RF24_250KBPS);
    radio.printDetails();
    Serial.print(F("--- END STUFF ---"));
}

void loop()
{
    if (Serial.available())
    {
        incomingValue = Serial.read();

        // Do all the processing here since this is the end of a line
        if (incomingValue == "TONRFSTART" || readStart)
        {
            if (i == 0 && readStart)
            {
                employeeRFID = incomingValue;
                i++;
            }
            if (i == 1 && readStart)
            {
                rfid = incomingValue;
                i++;
            }
            readStart = true;
        }

        if (incomingValue == "TONRFEND" && i == 2)
        {
            readStart = false;
            i = 0;
            employeeRFID = "";
            rfid = "";
        }
    }

    if (employeeRFID != "" && rfid != "")
    {
        Serial.print(employeeRFID + "=====" + rfid);
        sendData(rfid);
    }
}

void sendData(String data)
{
    // Length (with one extra character for the null terminator)
    int str_len = data.length() + 1;
    // Prepare the character array (the buffer)
    char char_array[str_len];
    // Copy it over
    data.toCharArray(char_array, str_len);
    radio.write(&char_array, sizeof(char_array));
    radio.powerDown();
    radio.powerUp();
}
