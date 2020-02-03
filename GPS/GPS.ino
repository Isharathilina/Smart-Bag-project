
#include "TinyGPS++.h"
TinyGPSPlus gps;


void setup() 
{ 
  Serial.begin(9600);
  Serial2.begin(9600); // for gps data 

}
 
void loop() 
{


Serial.println(getGpsData());


delay(1000);
  
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
	  
	  String gpsData="";
	  gpsData+= String(gps.location.lat());
	  gpsData+= String(",");
	  gpsData+= String(gps.location.lng());

	  return gpsData;
    }else{
      Serial.println("Initilize GPS.. ");
	  return "Initilize GPS.. ";
      
      }
  }else{
    
      Serial.println("No Serial data");
	  return "No Serial data";

    }




}
