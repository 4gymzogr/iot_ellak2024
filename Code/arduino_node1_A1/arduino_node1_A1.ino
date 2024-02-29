#include <LoRa.h>
#include "bsec.h"

// set serial communication speed
#define SERIAL_BAUD   115200

// Global variables
String output;
String json_message;
byte gateway_id = 0xDD;
byte node1_id = 0xAA;
unsigned long delay_time;
unsigned int ldr;
bool lSend = true;

// Functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);
void sendMessage(String, byte, byte, byte, byte);
String create_json();

// Create an object of the class Bsec
Bsec iaqSensor;

int counter = 0;
unsigned long int nDuration;

void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial);
 
  // Start BME688 IAQ sensor
  iaqSensor.begin(BME68X_I2C_ADDR_HIGH, Wire);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[13] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_GAS_PERCENTAGE
  };

  iaqSensor.updateSubscription(sensorList, 13, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();
  Serial.println("BME688 initializing sensor Ok!");

  // Start LoRa RFM95 communicator
  int n = 0;
  while (n < 10) {
    if (!LoRa.begin(868E6)) {
      Serial.println("Starting LoRa failed!");
    }
    else {
      Serial.println("LoRa RFM95 initializing Ok!");
      delay(1000);
      break;
    }
    delay(1000);
    n++;
  }
  if (n == 10) {
    Serial.println("LoRa RFM95 initializing failed.");
    while(true);
  }
}


void loop() {
  String json = create_json();
  Serial.println(json);

  sendMessage(json, gateway_id, node1_id, 0x3e, 0x4e);

  delay(3000);
}

// Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.bsecStatus != BSEC_OK) {
    if (iaqSensor.bsecStatus < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.bsecStatus);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BSEC warning code : " + String(iaqSensor.bsecStatus);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme68xStatus != BME68X_OK) {
    if (iaqSensor.bme68xStatus < BME68X_OK) {
      output = "BME68X error code : " + String(iaqSensor.bme68xStatus);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BME68X warning code : " + String(iaqSensor.bme68xStatus);
      Serial.println(output);
    }
  }
}


void errLeds(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}


// reads sensors and create json string
String create_json() {
  // Read LDR sensor
  //ldr = analogRead(A0);
  ldr = 568;

  json_message = "";

  // If new data is available on BME688
  if (iaqSensor.run()) { 
   
    json_message.concat("{\"Temperature\":");
    json_message.concat(String(iaqSensor.temperature));
    json_message.concat(",");
    
    json_message.concat("\"Pressure\":");
    json_message.concat(String(iaqSensor.pressure/100.0));
    json_message.concat(",");

    json_message.concat("\"Humidity\":");
    json_message.concat(String(iaqSensor.humidity));
    json_message.concat(",");

    json_message.concat("\"IAQ\":");
    json_message.concat(String(iaqSensor.iaq));
    json_message.concat(",");

    json_message.concat("\"CO2\":");
    json_message.concat(String(iaqSensor.co2Equivalent));
    json_message.concat(",");

    json_message.concat("\"Ldr\":");
    json_message.concat(String(ldr));
    json_message.concat("}"); 
  } 
  return json_message; 
}


void sendMessage(String json_message, byte gateway_id, byte node1_id, byte b1, byte b2) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(gateway_id);               // add destination address
  LoRa.write(node1_id);                    // add sender address
  //LoRa.write(msgCount);                 // add message ID
  LoRa.write(json_message.length());       // add payload length
  LoRa.write(b1);                       // add signature byte1
  LoRa.write(b2);                       // add signature byte2
  LoRa.print(json_message);                // add payload
  LoRa.endPacket();                     // finish packet and send it
}
