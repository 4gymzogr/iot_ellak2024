#include <LoRa.h>


int counter = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Sender");
  int n = 0;
  while (n < 10) {
    if (!LoRa.begin(868E6)) {
      Serial.println("Starting LoRa failed!");
    }
    else
    {
      Serial.println("Initializing LoRa Ok!");
      delay(2000);
      break;

    }
  
    delay(1000);
    n++;
  }
}


void loop() {
  byte b = random();
  Serial.print("Sending packet: ");
  Serial.print("Hello");
  Serial.print('-');
  Serial.print(counter);
  Serial.print('-');
  Serial.println(b);

  // send packet
  LoRa.beginPacket();
  LoRa.print("Hello");
  LoRa.write('-');
  LoRa.print(counter);
  LoRa.write('-');
  LoRa.print(b);
  Serial.print("Hello");
  LoRa.endPacket();  

  counter++;
  delay(1000);
}
