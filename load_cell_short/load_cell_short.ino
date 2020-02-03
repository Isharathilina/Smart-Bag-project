
#include <HX711.h>

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 21; //4
const int LOADCELL_SCK_PIN = 22;
float calibration_factor = 2125; //-7050 

HX711 scale;

void setup() {
  Serial.begin(57600);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
}

void loop() {
    //scale.set_scale(calibration_factor); //Adjust to this calibration factor

  if (scale.is_ready()) {
    long reading = scale.read();
    reading+=48300; // cal
    Serial.print("HX711 reading: ");
    Serial.println(reading);  //0.035274
  } else {
    Serial.println("HX711 not found.");
  }

  delay(1000);
  
}
