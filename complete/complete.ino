//include all necessary libraries: 

//TODO: calibrate K value for EC-meter
//TODO: clean up get_PPM, create initialize_pin function, optimize get_PH, prevent infinite loops for adjust functions
#include <RTC.h>
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
#define EC_Read A2
//digital
#define lights 8
#define EC_Power 7
#define air_temp_pin 6
#define water_temp_pin 2
//pumps
#define pump_1 3
#define pump_2 4
#define pump_3 5


//initialize thingies

//initialize pins
//settings
int distance = 30000;
//decide on minimum and maximum values
float ph_max = 6.7;
float ph_min = 6.0;

float ppm_min = 800;
float ppm_max = 1100;

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
float ppm = 0.0;

//declare functions
//measuring
float get_PPM();
float get_PH();
//adjusting
void adjust_PH();
void adjust_PPM();
void adjust_PUMP();


void setup() {
  Serial.begin(9600);
  
  RTC.begin();
  RTCTime startTime(1, Month::JANUARY, 2023, 0, 0, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_INACTIVE);
  RTC.setTime(startTime);

  sensors.begin();
  am2302.begin();
  ina219.begin();
  display.begin();
  pinMode(water_level_1, INPUT);
  pinMode(water_level_2, INPUT);
  pinMode(ph_meter, INPUT);
  pinMode(EC_Read, INPUT);

  pinMode(EC_Power, OUTPUT);
  digitalWrite(EC_Power, LOW);
  pinMode(air_temp_pin, INPUT);
  pinMode(water_temp_pin, INPUT);

  pinMode(pump_1, OUTPUT);
  digitalWrite(pump_1, LOW);
  pinMode(pump_2, OUTPUT);
  digitalWrite(pump_2, LOW);
  pinMode(pump_3, OUTPUT);
  digitalWrite(pump_3, LOW);
  pinMode(lights, OUTPUT);
  digitalWrite(lights, LOW);


  delay(2000);
  Serial.print("Sensors initialized. Beginning loop. Current settings: measurements taken every ");
  Serial.print(distance/1000);
  Serial.println(" Seconds");
  display.display();
}

void loop() {
  Serial.print("Measurement starting.");
  //control lights through time
  RTCTime currentTime;
  RTC.getTime(currentTime);

  int currentHour = currentTime.getHour();

  if((currentHour < 8 )||(currentHour > 20)){
    digitalWrite(lights, LOW);
  }
  else{
    digitalWrite(lights, HIGH);
  }
  //Declare measured variables
  water_level = 0;
  ph = 0;
  temp_water = 0;
  temp_air = 0;

  //Measure water temperature
  sensors.requestTemperatures();
  temp_water = sensors.getTempCByIndex(0);
    
  //Measure PH, since measurement is influenced by wall power, measure over one T (20ms)
  ph = get_PH(ph_meter);
  
  //Measure air temperature and humidity
  temp_air = am2302.get_Temperature();
  hum_air = am2302.get_Humidity();

  //Measure EC and PPM
  ppm = get_PPM(4, A2);

  //Check if values have to adjusted
  if(ph > ph_min){
    adjust_PH();
  }
  if(ppm < ppm_min){
    adjust_PPM();
  }
  delay(distance);

}


void update_display(){
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
}

float get_PPM(int EC_POWER, int EC_READ){
  float V_Water;
  float R_Water;
  float EC;
  float PPM;
  float K = 2.0;
  digitalWrite(EC_POWER, HIGH);

  delay(10);

  V_Water = analogRead(EC_READ);

  delay(10);

  digitalWrite(EC_Power, LOW);

  V_Water = V_Water * 5.0 /1024;
  R_Water = (1000*V_Water)/(5.0 - V_Water);
  EC = 1000/R_Water*K;
  PPM = EC*700;
  return PPM;
}

float get_PH(int ph_pin){
  float res = 0.0;
  float ph = 0.0;
  for(int i = 0; i < 10; i++){
    res += analogRead(ph_pin);
    delay(2);
  }
  res/=10;
  res = res * 5.0 / 1024.0;
  ph = res * 3.5 + 0.15;
  return ph;
}

void adjust_PH(){
  float ph = 0.0;
  ph = get_PH(ph_meter);

  while(ph > ph_max){
    digitalWrite(pump_2, HIGH);
    delay(6000);
    digitalWrite(pump_2, LOW);
    delay(30000);
    ph = get_PH(ph_meter);
  }
}

void adjust_PPM(){
  float ppm = get_PPM(EC_Power, EC_Read);

  while(ppm < ppm_max){
    digitalWrite(pump_3, HIGH);
    delay(6000);
    digitalWrite(pump_3, LOW);
    delay(30000);
    ppm = get_PPM(EC_Power, EC_Read);
  }
}