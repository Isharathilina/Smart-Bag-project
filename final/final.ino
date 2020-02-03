
#include <WiFi.h>
#include "PubSubClient.h"
 
const char* ssid = "Cloud"; // Enter your WiFi name
const char* password =  "11115555"; // Enter WiFi password
const char* mqttServer = "soldier.cloudmqtt.com";
const int mqttPort = 12729;
const char* mqttUser = "ciyajbsc";
const char* mqttPassword = "je1T5MADj3Wi";

WiFiClient espClient;
PubSubClient client(espClient);


//------------------
#include <SPI.h>
#include <MFRC522.h>

// ifid circuit wiring spi bus + 
#define SS_PIN 12
#define RST_PIN 13 //22
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; 
// Init array that will store new NUID 
byte nuidPICC[4];

//--------------------

#include <HX711.h>

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 21; //4
const int LOADCELL_SCK_PIN = 22;
float calibration_factor = 2125; //-7050 

HX711 scale;
//----------------

#include "TinyGPS++.h"
TinyGPSPlus gps;



void setup() 
{ 
  Serial.begin(9600);
  Serial2.begin(9600); // for gps data 

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);  // load cell start
  
  
  
  
  
  WiFi.begin(ssid, password);
 
  if (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.println("Not connect to wifi");
  }
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  if (!client.connected()) 
  {
    Serial.println("Not connect to MQTT ");
 
    if (client.connect("ESPclient", mqttUser, mqttPassword )) {
 
      Serial.println("connected");  
 
    } else
  {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      
    }
  }
 
  client.publish("send", "System started");
  client.subscribe("getData");


}
 
void loop() 
{
  client.loop(); // wait incoming data
  tryToConnect(); // try to connect server
  


//Serial.println(GetRfidUuid()); //getLoadCellData
//Serial.println(getLoadCellData()); //getLoadCellData
Serial.println(getGpsData());

  client.publish("send", "System started");



delay(1000);
  
}




void tryToConnect()
{
  if (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.println("Not connect to wifi");
  }
  
    if (!client.connected()) 
  {
    Serial.println("Not connect to MQTT ");
 
    if (client.connect("ESPclient", mqttUser, mqttPassword )) {
 
      Serial.println("connected");  
 
    } else
  {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      
    }
  }



}
 
void callback(char* topic, byte* payload, unsigned int length)
{
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}


String getGpsData()
{
  
 if(Serial2.available() > 0)
 {
    gps.encode(Serial2.read());
    if (gps.location.isUpdated())
  {
      Serial.print("Latitude= "); 
      Serial.print(gps.location.lat(), 6);
      Serial.print(" Longitude= "); 
      Serial.println(gps.location.lng(), 6);
    
    return "data";
    }else{
    Serial.println("Initialize GPS");

  
  }
  }else{
    Serial.println("No serial data");

  
  }




}



long getLoadCellData()
{
  //scale.set_scale(calibration_factor); //Adjust to this calibration factor
  if (scale.is_ready()) 
  {
    long reading = scale.read();
    reading+=48300; // cal
    Serial.print("HX711 reading: ");
    Serial.println(reading);  //0.035274
  return reading;
  } else {
    Serial.println("HX711 not found.");
  }

}



String GetRfidUuid()
{

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return "F";

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return "F";

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) 
  {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return "F";
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) 
  {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    String uuidData="";
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
      uuidData+= String(rfid.uid.uidByte[i]);
    }
   
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();

    //String strData=String ((char*)rfid.uid.uidByte);
    Serial.print("UUID string data:-");
    Serial.println(uuidData);
  
  return uuidData;
  
  }
  else Serial.println(F("Card read previously."));

  // Halt PICC
  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
  
  return "N";

}

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) 
{
  for (byte i = 0; i < bufferSize; i++) 
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) 
{
  for (byte i = 0; i < bufferSize; i++) 
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
