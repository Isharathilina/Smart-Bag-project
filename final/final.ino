
#include <WiFi.h>
#include "PubSubClient.h"
#include <SPI.h>
#include <MFRC522.h>
#include <HX711.h>
#include "TinyGPS++.h"
#include <EEPROM.h>

#define EEPROM_SIZE 1
#define ageDataAddr 0

#define buzerOutPin 34

#define DEBUG
 
const char* ssid = "Cloud"; // Enter your WiFi name
const char* password =  "11115555"; // Enter WiFi password
const char* mqttServer = "soldier.cloudmqtt.com";
const int mqttPort = 12729;
const char* mqttUser = "ciyajbsc";
const char* mqttPassword = "je1T5MADj3Wi";

WiFiClient espClient;
PubSubClient client(espClient);

// rfid pins
#define SS_PIN 12
#define RST_PIN 13 
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; 
// Init array that will store new NUID 
byte nuidPICC[4];

// load cell pins
const int LOADCELL_DOUT_PIN = 21; //4
const int LOADCELL_SCK_PIN = 22;
float calibration_factor = 2125; //-7050 

HX711 scale;
TinyGPSPlus gps;

// serial 2 connect to gps


void setup() 
{ 
  Serial.begin(9600);
  Serial2.begin(9600); // for gps data 
  EEPROM.begin(EEPROM_SIZE);

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);  // load cell start
  
  WiFi.begin(ssid, password);
  pinMode(buzerOutPin, OUTPUT); // buzzer output
 
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
  
  
  Serial.println(GetRfidUuid()); //getLoadCellData str
  Serial.println(getLoadCellData()); //getLoadCellData lng
  Serial.println(getGpsData()); // str

  checkBagWeight(getLoadCellData());
  
  char rfidDataBuf[30];
  GetRfidUuid().toCharArray(rfidDataBuf, 30);

  client.publish("send", rfidDataBuf);
  
  Sendsms("0000000000", getGpsData());


  delay(1000);
  
}



void checkBagWeight(long BagWeightRow)
{
	  int childAge = EEPROM.read(ageDataAddr);
	  Serial.println(childAge);
	  
	  int bagWeight = BagWeightRow/5000;
	  if(bagWeight>childAge*(3/10))
	  {
		digitalWrite(buzerOutPin, HIGH);
		delay(500);
		digitalWrite(buzerOutPin, LOW);
		delay(500);

	  }else
	  {
		digitalWrite(buzerOutPin, LOW);
	  }

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

  //client.publish("flavaSMS", "data received..!");
   
  #ifdef DEBUG
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  #endif
  
  char* dataArry = (char*)malloc(length);
 
  #ifdef DEBUG
  Serial.print("Message:");
  #endif
  for (int i = 0; i < length; i++)
  {
    dataArry[i] = (char)payload[i];

  }

  #ifdef DEBUG
  Serial.print("length ");
  Serial.println(length);
  #endif

  Serial.println(dataArry);
  
   // start converting
  String *strData;
  strData = new String[length];

  int count =0;
  char * pch;
  pch = strtok (dataArry,"=");
  
  while (pch != NULL)
  {
    String tempst= pch;
    strData[count] = tempst;
    #ifdef DEBUG
    printf ("%s\n",pch);
    #endif
    pch = strtok (NULL, "=");
    count++;
  }
  
  Serial.print("Name :- ");
  Serial.println(strData[0]);
  Serial.print("Age data :- ");
  Serial.println(strData[1]);
  //Sendsms(strData[0],strData[1]);
  
  EEPROM.write(ageDataAddr, strData[1].toInt());
  EEPROM.commit();

 free(dataArry);
}
 



String getGpsData()
{
  
 if(Serial2.available() > 0)
 {
	for(int i=0; i<10; i++)
	{
	
		gps.encode(Serial2.read());
		if (gps.location.isUpdated())
		{
		  Serial.print("Latitude= "); 
		  Serial.print(gps.location.lat(), 6);
		  Serial.print(" Longitude= "); 
		  Serial.println(gps.location.lng(), 6);
		  
		  String gpsData;
		  gpsData+= String(gps.location.lat());
		  gpsData+= ",";
		  gpsData+= String(gps.location.lng());

		
		return gpsData;
		break;
		}else
		{
			Serial.println("Initialize GPS");
		}
	
	}
   
  }else
  {
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

// AT+CMGR=1\r    // read 1st sms
// AT+CNMI=1,2,0,0,0  // enable not

int Sendsms(String num, String msg){

  Serial2.print("AT\r\n");
  delay(500);
  while(!Serial2.available());  //ready gsm
  
  while(Serial2.available())
  {   //garbadge collection
    //Serial.print((char)Serial2.read());
    int temp = (char)Serial2.read();
  }
    
  Serial2.print("AT+CMGF=1\r\n");   // Text Mode
  delay(100);
  
  Serial2.print("AT+CMGS=\"");
  Serial2.print(num);
  Serial2.print("\"\r\n");
  delay(100);
  
  Serial2.println(msg);
  delay(50);
  Serial2.println((char)26);    //CTRL+Z  for end msg
  delay(50);
    
  Serial.print("Msg sent to ");
  Serial.print(num);
  Serial.print(" \r\n");


  return 1;
  
}