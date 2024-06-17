//include all necessary libraries: 

//Water_temp
#include <DallasTemperature.h>
#include <OneWire.h>

//Air_temp
#include <AM2302-Sensor.h>

//i2c Devices
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_SSD1306.h>

//define pins
//analog
#define water_level_1 A0
#define water_level_2 A1
#define ph_meter A3
//digital
#define air_temp_pin 1
#define water_temp_pin 2
//pumps
#define pump_1 3
#define pump_2 4
#define pump_3 5


//initialize thingies

//settings
int samples = 10;
int time = 2000;

//Air temperature and humidity
AM2302::AM2302_Sensor am2302(air_temp_pin);

//Water temperature
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//Conductivity 
Adafruit_INA219 ina219;

//declare variables
int water_level = 0;
float ph = 0.0;
float temp_water = 0.0;
float temp_air = 0.0;
float hum_air = 0.0;
float ec = 0.0;

//declare functions
int get_water_level();
float get_ph();
float get_ec();

void setup() {
  Serial.begin(9600);
  sensors.begin();
  am2302.begin();
  ina219.begin();
  delay(2000);
  Serial.println("Sensors initialized. Beginning loop. Current settings: averaging ")
  Serial.print(samples)
  Serial.print(" samples over ");
  Serial.print(time/1000)
  Serial.println(" seconds.")
}

void loop() {
  Serial.println("Loop starting.Defining variables...");
  water_level = 0;
  ph = 0;
  temp_water = 0;
  temp_air = 0;

  for(int i = 0; i < samples; i++){
    
  }

}
