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
#include <Adafruit_GFX.h>

//define pins
//analog
#define water_level_1 A0
#define water_level_2 A1
#define ph_meter A3
//digital
#define air_temp_pin 3
#define water_temp_pin 2
//pumps
#define pump_1 3
#define pump_2 4
#define pump_3 5


//initialize thingies

//settings
int samples = 10;
int time_1 = 10000;

//Air temperature and humidity
AM2302::AM2302_Sensor am2302(air_temp_pin);

//Water temperature
OneWire oneWire(water_temp_pin);
DallasTemperature sensors(&oneWire);

//Conductivity 
Adafruit_INA219 ina219;

//Display
Adafruit_SSD1306 display(128,32,&Wire,-1);
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
  display.begin();
  delay(2000);
  Serial.println("Sensors initialized. Beginning loop. Current settings: averaging ");
  Serial.print(samples);
  Serial.print(" samples over ");
  Serial.print(time_1 / 1000);
  Serial.println(" seconds.");
  display.display();
}

void loop() {
  Serial.print("Measurement starting.");
  water_level = 0;
  ph = 0;
  temp_water = 0;
  temp_air = 0;

  for(int i = 0; i < samples; i++){
    sensors.requestTemperatures();
    temp_water += sensors.getTempCByIndex(0);
    delay(time_1/samples);
    Serial.print(".");
  }
  float res = 0.0;
  for(int i = 0; i < 10; i++){
    res += analogRead(A3);
    delay(2);
  }
  res/=10;
  res = res * 5.0 / 1024.0;
  ph = res * 3.5 + 0.15;
  
  
  temp_water = temp_water/samples;
  temp_air = am2302.get_Temperature();
  hum_air = am2302.get_Humidity();
  Serial.println(".");
  Serial.println("Measurement finished.");
  Serial.print("Air temperature: ");
  Serial.println(temp_air);
  Serial.print("Water temperature: ");
  Serial.println(temp_water);
  Serial.print("PH-Value: ");
  Serial.println(ph);
  
  display.clearDisplay();
  display.setTextSize(0);
  display.setCursor(0,0);
  display.setTextColor(SSD1306_WHITE);
  display.print("Water temp: ");
  display.println(temp_water);
  display.print("Air temp: ");
  display.println(temp_air);
  display.print("PH-Value: ");
  display.println(ph);
  display.print("Humidity: ");
  display.println(hum_air);
  display.display();
  
  delay(5000);

}
