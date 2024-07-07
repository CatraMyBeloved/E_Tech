// Include all necessary libraries

// TODO: consider temperature in ppm

#include <RTC.h> // Real Time Clock library
#include <DallasTemperature.h> // Library for water temperature sensor
#include <OneWire.h> // Library for one-wire communication
#include <AM2302-Sensor.h> // Library for air temperature and humidity sensor
#include <Wire.h> // Library for I2C communication
#include <Adafruit_SSD1306.h> // Library for OLED display
#include <Adafruit_GFX.h> // Graphics library for OLED display
#include <BH1750.h> //Brightness sensor

#define NUMBER_SENSORS 5
// Define pins
// Analog pins
#define PIN_WATER_LVL_1 A0
#define PIN_WATER_LVL_2 A1
#define PIN_PH_METER A3
#define PIN_EC_READ A2
// Digital pins
#define PIN_LIGHTS 8
#define PIN_EC_POWER 7
#define PIN_AIR_TEMP 6
#define PIN_WATER_TEMP 2
#define ROTARY_A 9
#define ROTARY_B 10
#define ROTARY_SWITCH 11
// Pump pins
#define PIN_PUMP_WATER 3
#define PIN_PUMP_NUTRIENTS 4
#define PIN_PUMP_PH_DOWN 5

// Measurement interval in milliseconds 
#define MEASUREMENT_INTERVAL 30000
// Ranges for PH and PPM
#define PH_RANGE 0.5
#define PPM_RANGE 150
// Resolutions for rotation thingies
#define PH_RESOLUTION  0.1
#define PPM_RESOLUTION  50
// Light levels
#define LIGHT_MIN 200
#define LIGHT_MAX 30000
// Constants for EC and pH measurements
#define V_REF 5.0
#define VOLTAGE_RESOLUTION 1024.0
#define PPM_CONST 700.0
#define R1 1000
#define K 1.85
#define PH_FACTOR 3.5 // conversion factor mV PH
#define PH_OFFSET 0.15
#define PPM_CYCLES 6
// Adjustment parameters
#define MAX_ADJUSTMENT_LOOPS 10
#define PH_PUMP_AMOUNT 6000 // Pump run time in milliseconds
#define PPM_PUMP_AMOUNT 6000 // Pump run time in milliseconds
#define SENSOR_WAIT 10000 // Wait time in milliseconds

// Light control hours
#define PUMP_ON_HOUR 7
#define PUMP_OFF_HOUR 21

// Air temperature and humidity sensor
AM2302::AM2302_Sensor am2302(PIN_AIR_TEMP);

// Water temperature sensor
OneWire oneWire(PIN_WATER_TEMP);
DallasTemperature sensors(&oneWire);

// OLED display
Adafruit_SSD1306 display(128, 32, &Wire, -1);

// Brightness sensor
BH1750 lightMeter;

// Variables limits
float desired_ppm = 0.0;
float desire_ph = 0.0;

// Declare variables for sensor readings
int water_level = 0;
int pump_state = 0;
int light_state = 0;
int ph_ok = 0; 
int ppm_ok = 0;
// Declare variable limits for ph and ppm, chose through the rotary knobs
float ph_max = 0.0;
float ph_min = 0.0;
float ppm_max = 0.0;
float ppm_min = 0.0;
// Declare variables for measurement
float ph = 0.0;
float temp_water = 0.0;
float temp_air = 0.0;
float hum_air = 0.0;
float ppm = 0.0;
float ppm_temp = 0.0;
int ppm_counter = 0;
float light_level = 0.0;
// Variables to determine the entered values
int ph_set_counter = 0;
int ppm_set_counter = 0;

// Function declarations
float get_PPM();
float get_PH();
float get_light_level();
void set_values();
void validate(float measured_values[]);
void adjust_PH();
void adjust_PPM();
void initialize_pin(int pin);
void initialize_input_pin(int pin);
void update_display();
void control_pump(int currentHour);
void control_light(float light_level);

void update_display_setup_ph(int counter);
void update_display_setup_PPM(int counter);
void set_values();
//validation of measured values
float valid_values[NUMBER_SENSORS][2] = {
  {0, 40},//air temperature
  {0, 100}, //water temperature
  {0, 14}, //ph 
  {0, 3000} //ec
}; 

float measured_values[NUMBER_SENSORS] = {};
int validated[NUMBER_SENSORS] = {};

const char *sensor_names[] = {
  "air temperature",
  "water temperature",
  "ph value",
  "PPM value"
};

int error_found = 0;
void setup() {
  Serial.begin(9600); // Initialize serial communication
  Wire.begin();

  // Begin real-time clock, set start time
  RTC.begin();
  RTCTime startTime(1, Month::JANUARY, 2023, 22, 12, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_INACTIVE);
  RTC.setTime(startTime);
  
  // Initialize sensors and display
  sensors.begin();
  am2302.begin();
  display.begin();
  display.display();
  lightMeter.begin();

  // Initialize input pins
  initialize_input_pin(PIN_WATER_LVL_1);
  initialize_input_pin(PIN_WATER_LVL_2);
  initialize_input_pin(PIN_PH_METER);
  initialize_input_pin(PIN_EC_READ);
  initialize_input_pin(PIN_AIR_TEMP);
  initialize_input_pin(PIN_WATER_TEMP);
  initialize_input_pin(ROTARY_A);
  initialize_input_pin(ROTARY_B);
  initialize_input_pin(ROTARY_SWITCH);
  // Initialize output pins
  initialize_pin(PIN_PUMP_WATER);
  initialize_pin(PIN_PUMP_NUTRIENTS);
  initialize_pin(PIN_PUMP_PH_DOWN);
  initialize_pin(PIN_LIGHTS);
  initialize_pin(PIN_EC_POWER);

  delay(2000); // Wait for sensors to stabilize
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.print("Sensors initialized. Beginning loop. Current settings: measurements taken every ");
  display.print(MEASUREMENT_INTERVAL / 1000);
  display.println(" Seconds");
  
  display.display(); // Display initial message

  set_values();

  ph_min = ph_set_counter * PH_RESOLUTION - PH_RANGE;
  ph_max = ph_set_counter * PH_RESOLUTION + PH_RANGE;

  ppm_min = ppm_set_counter * PPM_RESOLUTION - PPM_RANGE;
  ppm_max = ppm_set_counter * PPM_RESOLUTION + PPM_RANGE;

  delay(10000);
}

void loop() {
  error_found = 0;
  Serial.println("Measurement starting.");
  
  // Use real-time clock to control PIN_LIGHTS, get current time
  RTCTime currentTime;
  RTC.getTime(currentTime);
  
  // Get current hour
  int currentHour = currentTime.getHour();
  
  // Control pump based on current hour
  control_pump(currentHour);
  
  // Measure light level to control lights, and control lights
  light_level = lightMeter.readLightLevel();
  control_lights(light_level);

  // Measure water temperature
  sensors.requestTemperatures();
  temp_water = sensors.getTempCByIndex(0);
  measured_values[1] = temp_water;

  // Measure pH value
  ph = get_PH();
  measured_values[2] = ph;

  // Measure air temperature and humidity
  temp_air = am2302.get_Temperature();
  hum_air = am2302.get_Humidity();
  measured_values[0] = temp_air;

  // Measure EC and PPM
  if(ppm_counter >= PPM_CYCLES){
    ppm = ppm_temp/PPM_CYCLES;
    ppm_counter = 0;
    ppm_temp = 0;
  }
  else{
    ppm_temp+= get_PPM();
    ppm_counter++;
    delay(5);
  }
  
  measured_values[3] = ppm;
  
  //validate measured values
  display.clearDisplay();
  display.display();
  validate(measured_values);
  for(int i = 0; i < NUMBER_SENSORS; i++){
    if(!validated[i]){
      display.print("Invalid value for ");
      display.println(sensor_names[i]);
      error_found = 1;
    }
  }
  if(error_found == 0){
  //set ppm_ to 0 if ppm is too high (low ppm is handled by adjustment functions)
  if(ppm > 1100){
    ppm_ok = 0;
  }
  else{
    ppm_ok = 1;
  }
  // set ph_ok flag to 0 if ph is too low (high ph is handled by adjustment functions)
  if(ph < 1){
    ph_ok = 0;
  }
  else{
    ph_ok = 1;
  }
  // Check if pH needs adjustment
  if (ph > ph_max) {
    
    adjust_PH();
    
    return;
  }
  
  // Check if PPM needs adjustment
  if (ppm < ppm_min) {
    
    adjust_PPM();
    
    return;
  }

  // Update the display with current sensor values
  update_display();
  }

  Serial.println(ph);
  Serial.println(ppm);
  Serial.println(temp_water);
  Serial.println(temp_air);
  Serial.println(ph_ok);
  
  delay(MEASUREMENT_INTERVAL); // Wait for the next measurement cycle
}

void update_display() {
  // Clear the display
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  // Set text size and color
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  // Print sensor values on the display
  display.print("Water temp: ");
  display.println(temp_water);
  display.print("Air temp: ");
  display.println(temp_air);
  display.print("PH-Value: ");
  display.println(ph);
  display.print("PPM: ");
  display.println(ppm);
  if(ph_ok == 0 || ppm_ok == 0){
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE);
    if(ph_ok == 0){
      display.println("PH not in recommended range.");
      display.println(ph);
    }
    if(ppm_ok == 0){
        display.println("PPM not in recommended range.");
        display.println(ppm);
    }
}  
  display.display(); // Update the display with new data
}

float get_PPM() {
  digitalWrite(PIN_EC_POWER, HIGH); // Power the EC meter
  delay(10); // Wait for stable reading
  
  // Get voltage drop across the water and convert to millivolts
  float V_Water = analogRead(PIN_EC_READ) * V_REF / VOLTAGE_RESOLUTION;
  
  // Turn off power to avoid electrolysis
  digitalWrite(PIN_EC_POWER, LOW);
  
  // Calculate resistance of water
  float R_Water = (R1 * V_Water) / (V_REF - V_Water);
  
  // Calculate electrical conductivity in milliSiemens
  float EC = R1 / R_Water * K;
  
  // Convert EC to PPM
  float PPM = EC * PPM_CONST;
  
  return PPM;
}

float get_PH() {
  float res = 0.0;
  
  // Measure pH over one period of the 50 Hz pump (20 ms) to average out noise
  for (int i = 0; i < 10; i++) {
    res += analogRead(PIN_PH_METER);
    delay(2);
  }
  
  // Convert analog value to pH
  res = (res / 10 * V_REF / VOLTAGE_RESOLUTION * PH_FACTOR) + PH_OFFSET;
  
  return res;
}

void adjust_PH() {
  int number_loops = 0; // Initialize loop counter
  
  // Adjust pH until it's within the desired range or max loops reached
  while (get_PH() > ph_min && number_loops < MAX_ADJUSTMENT_LOOPS) {
    // Turn on pH-down pump for 6 seconds
    Serial.println("adjusting ph");
    Serial.println(ph);
    digitalWrite(PIN_PUMP_PH_DOWN, HIGH);
    delay(PH_PUMP_AMOUNT);
    digitalWrite(PIN_PUMP_PH_DOWN, LOW);
    
    // Wait 30 seconds for pH to adjust
    delay(SENSOR_WAIT);
    number_loops++;
  }
}

void adjust_PPM() {
  int number_loops = 0; // Initialize loop counter
  
  // Adjust PPM until it's within the desired range or max loops reached
  while (ppm < ppm_max && number_loops < MAX_ADJUSTMENT_LOOPS) {
    // Turn on nutrient pump for 6 seconds
    Serial.println("Adjusting ppm");
    Serial.println(ppm);
    digitalWrite(PIN_PUMP_NUTRIENTS, HIGH);
    delay(PPM_PUMP_AMOUNT);
    digitalWrite(PIN_PUMP_NUTRIENTS, LOW);
    
    // Wait 30 seconds for nutrients to mix
    delay(SENSOR_WAIT);
    ppm = get_PPM();
    number_loops++;
  }
}
void control_lights(float light_level) {
  if(light_level < LIGHT_MIN && light_state == 0){
    light_state = 1;
    digitalWrite(PIN_LIGHTS, HIGH);
  }
  else if(light_level > LIGHT_MAX && light_state == 1){
    light_state = 0;
    digitalWrite(PIN_LIGHTS, LOW);
  }
}
void control_pump(int currentHour) {
  // Turn off lights if current hour is outside the range
  if ((currentHour < PUMP_ON_HOUR || currentHour > PUMP_OFF_HOUR) && pump_state == 1) {
    digitalWrite(PIN_PUMP_WATER, LOW);
    pump_state = 0;
  } 
  // Turn on lights if current hour is within the range
  else if ((currentHour >= PUMP_ON_HOUR && currentHour <= PUMP_OFF_HOUR) && pump_state == 0) {
    digitalWrite(PIN_PUMP_WATER, HIGH);
    pump_state = 1;
  }
}

void initialize_pin(int pin) {
  pinMode(pin, OUTPUT); // Set pin as output
  digitalWrite(pin, LOW); // Ensure pin is initially low
}

void initialize_input_pin(int pin) {
  pinMode(pin, INPUT); // Set pin as input
}

void validate(float measured_values[]){
  for(int i = 0; i < NUMBER_SENSORS; i++){
    if(measured_values[i] >= valid_values[i][0] && measured_values[i] <= valid_values[i][1]){
      validated[i] = 1;
    }
  }
}

void update_display_setup_ph(int counter){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Set PH");
  display.println(counter * PH_RESOLUTION);
  display.display();
}

void update_display_setup_PPM(int counter){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Set PPM");
  display.println(counter * PPM_RESOLUTION);
  display.display();
}

void set_values(){
  //Display initial message
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Desired PH?");
  display.display();
  //declare needed variables
  int lastState = 0;
  int currentState = 0;
  //save the t 0 state to laststate
  lastState = digitalRead(ROTARY_A);
  //while the button is not pressed, look for rotation
  while(digitalRead(ROTARY_SWITCH) == HIGH){
    currentState = digitalRead(ROTARY_A);  // Read the current state of CLK
    if (currentState != lastState  && currentState == 1) { // if the new state is different from the state before, and = 1(high)
      if (digitalRead(ROTARY_B) != currentState) { //check the other signal, if its 0 turn clockwise
        ph_set_counter --;
        update_display_setup_ph(ph_set_counter);
      } else {                                     //otherwise turn counterclockwise
        ph_set_counter ++;
        update_display_setup_ph(ph_set_counter);
      }
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Desired PPM?");
  display.display();
  
  lastState = digitalRead(ROTARY_A); //repeat the same thing as above

  while(digitalRead(ROTARY_SWITCH) == HIGH){
    currentState = digitalRead(ROTARY_A);  // Read the current state of CLK
    if (currentState != lastState  && currentState == 1) {
      if (digitalRead(ROTARY_B) != currentState) {
        ppm_set_counter --;
        update_display_setup_ph(ppm_set_counter);
      } else {
        ppm_set_counter ++;
        update_display_setup_ph(ppm_set_counter);
      }
    }
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Settings succesfully chosen.");
  display.print("PH: ");
  display.println(ph_set_counter * PH_RESOLUTION);
  display.print("PPM: ");
  display.println(ppm_set_counter * PPM_RESOLUTION);
  display.display();
}