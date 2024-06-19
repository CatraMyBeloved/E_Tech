#include "OneWire.h"
#include "DallasTemperature.h"

#define ONE_WIRE_BUS 2

#define number_sensors 1
OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

float getTemp();
float result = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  sensors.begin();
  Serial.println("Hihihihi");
  

}

void loop() {
  result = getTemp(100, 2000);
  Serial.println(result);
  delay(2000);

}


float getTemp(int num_samples, int time){
  
  float res = 0;
  for(int i = 0; i < num_samples; i++){
    sensors.requestTemperatures();

    res += sensors.getTempCByIndex(0);
    delay(time/num_samples);
  }
  return res/num_samples;
}