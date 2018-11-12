#include <Wire.h>


void setup() {
  Wire.begin(8);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(9600);  // initialize serial communication
  Serial.println("---INIT---");
}

void loop () {
  delay(100);
}
void receiveEvent(int howMany)
{
  String productId;
  String EmpId;
  bool checkAdmin = false;
  while (Wire.available() > 0) // loop through all but the last
  {
    char data = Wire.read(); // receive byte as a character
    if (data == "U") {
      checkAdmin = true;
    } else {      
      if (checkAdmin) {
        EmpId.concat(data);
      }else{
        productId.concat(data);
      }
      
    }

  }
  Serial.println("==start ==");
  Serial.println(productId);
  Serial.println(EmpId);
  Serial.println("==end==");
  delay(500);
}
