#include <LoRa.h>
#include <MQ135.h>
#include "bsec.h"

// set serial communication speed
#define SERIAL_BAUD   115200
#define GATEWAY   0xDD
//#define NODE      0xAA
//#define NODE      0xBB
#define NODE      0xCC
#define PIN_MQ135 A0
#define GREENLED    7
#define YELLOWLED   6
#define REDLED      5



// Global variables
unsigned int ldr, dataLength;
byte byte1, byte2;
char*   total;
String output, json_msg = "";
int greenLed, yellowLed, redLed;


// Functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);
void sendMessage(String, byte, byte, byte, byte);
String create_json(void);
void InitBME688();
void InitLoRaRFM95();


// Create objects
Bsec iaqSensor;
MQ135 mq135_sensor(PIN_MQ135);

void InitBME688() {
  // Start BME688 IAQ sensor
  Serial.println("Attempting to init BME688...");
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
}


void InitLoRaRFM95() {
  // Start LoRa RFM95 communicator
  Serial.println("Attempting to init LoRaRFM95...");
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


float GetDataFromMQ135(float temperature, float humidity) {
  float rzero = mq135_sensor.getRZero();
  float correctedRZero = mq135_sensor.getCorrectedRZero(temperature, humidity);
  float resistance = mq135_sensor.getResistance();
  float ppm = mq135_sensor.getPPM();
  float correctedPPM = mq135_sensor.getCorrectedPPM(temperature, humidity);

  /*Serial.print("MQ135 RZero: ");
  Serial.print(rzero);
  Serial.print("\t Corrected RZero: ");
  Serial.print(correctedRZero);
  Serial.print("\t Resistance: ");
  Serial.print(resistance);
  Serial.print("\t PPM: ");
  Serial.print(ppm);
  Serial.print("\t Corrected PPM: ");
  Serial.print(correctedPPM);
  Serial.println("ppm");*/

  if (correctedPPM <= 150){
    greenLed = HIGH;
    yellowLed = LOW;
    redLed = LOW;
  } else if(correctedPPM <= 400) {
    greenLed = LOW;
    yellowLed = HIGH;
    redLed = LOW;
  } else {
    greenLed = LOW;
    yellowLed = LOW;
    redLed = HIGH;
  }
  digitalWrite(GREENLED, greenLed);
  digitalWrite(YELLOWLED, yellowLed);
  digitalWrite(REDLED, redLed);

  return correctedPPM;
}


void setup() {
  pinMode( GREENLED, OUTPUT);
  pinMode( YELLOWLED, OUTPUT);
  pinMode( REDLED, OUTPUT);
  
  Serial.begin(SERIAL_BAUD);
  //while (!Serial);

  delay(3000);
  InitBME688();
  InitLoRaRFM95();
}


void loop() {
  
  json_msg = create_json();

  // checkSum for errors
  int msgSum = 0;
  for (int i=0; i<json_msg.length(); i++) {
    //msgSum += json_msg.charAt(i)*(i+1);
    msgSum += json_msg.charAt(i);
  }
      
  byte byte1 = msgSum / 256;
  byte byte2 = msgSum % 256;
    
  dataLength = json_msg.length();
  dataLength ++;
      
  //char total[dataLength];
  //memset(total, '\0', sizeof(dataLength));
  //json_msg.toCharArray(total, dataLength);
  Serial.println(json_msg);
  sendMessage(json_msg, byte1, byte2);

  delay(random(3, 7)*1000);
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
String create_json(void) {
  // Read LDR sensor
  ldr = analogRead(A1);

  String json_message = "";

  // If new data is available on BME688
  if (iaqSensor.run()) { 

    float iaq = GetDataFromMQ135(iaqSensor.temperature, iaqSensor.humidity);

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
    json_message.concat(String(iaq));
    json_message.concat(",");

    /*json_message.concat("\"CO2\":");
    json_message.concat(String(iaqSensor.co2Equivalent));
    json_message.concat(",");*/

    json_message.concat("\"Ldr\":");
    json_message.concat(String(ldr));
    json_message.concat("}"); 
  } 
  return json_message; 
}


void sendMessage(String json_message, byte b1, byte b2) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(GATEWAY);               // add destination address
  LoRa.write(NODE);                    // add sender address
  //LoRa.write(msgCount);                 // add message ID
  LoRa.write(json_message.length());       // add payload length
  LoRa.write(b1);                       // add signature byte1
  LoRa.write(b2);                       // add signature byte2
  LoRa.print(json_message);                // add payload
  delay(30);
  LoRa.endPacket();                     // finish packet and send it
}
