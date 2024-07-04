// Include all necessary libraries

// TODO: calibrate K value for EC-meter, error handling
#include <RTC.h> // Real Time Clock library
#include <DallasTemperature.h> // Library for water temperature sensor
#include <OneWire.h> // Library for one-wire communication
#include <AM2302-Sensor.h> // Library for air temperature and humidity sensor
#include <Wire.h> // Library for I2C communication
#include <Adafruit_SSD1306.h> // Library for OLED display
#include <Adafruit_GFX.h> // Graphics library for OLED display

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
// Pump pins
#define PIN_PUMP_WATER 3
#define PIN_PUMP_NUTRIENTS 4
#define PIN_PUMP_PH_DOWN 5

// Measurement interval in milliseconds 
#define MEASUREMENT_INTERVAL 30000
// pH limits
#define PH_MAX 14
#define PH_MIN 1
// PPM limits
#define PPM_MIN 0
#define PPM_MAX 100100

// Constants for EC and pH measurements
#define V_REF 5.0
#define VOLTAGE_RESOLUTION 1024.0
#define PPM_CONST 700.0
#define R1 1000
#define K 1.85
#define PH_FACTOR 3.5
#define PH_OFFSET 0.15
#define PPM_CYCLES 6
// Adjustment parameters
#define MAX_ADJUSTMENT_LOOPS 10
#define PH_PUMP_AMOUNT 6000 // Pump run time in milliseconds
#define PPM_PUMP_AMOUNT 6000 // Pump run time in milliseconds
#define SENSOR_WAIT 10000 // Wait time in milliseconds

// Light control hours
#define LIGHTS_ON_HOUR 7
#define LIGHTS_OFF_HOUR 21

// Air temperature and humidity sensor
AM2302::AM2302_Sensor am2302(PIN_AIR_TEMP);

// Water temperature sensor
OneWire oneWire(PIN_WATER_TEMP);
DallasTemperature sensors(&oneWire);

// OLED display
Adafruit_SSD1306 display(128, 32, &Wire, -1);

// Declare variables for sensor readings
int water_level = 0;
int light_state = 0;
int ph_ok = 0; 
int ppm_ok = 0;
float ph = 0.0;
float temp_water = 0.0;
float temp_air = 0.0;
float hum_air = 0.0;
float ppm = 0.0;
float ppm_temp = 0.0;
float ppm_counter = 0.0;

// Function declarations
float get_PPM();
float get_PH();
void validate(float measured_values[]);
void adjust_PH();
void adjust_PPM();
void initialize_pin(int pin);
void initialize_input_pin(int pin);
void update_display();
void control_lights(int currentHour);
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
  
  // Begin real-time clock, set start time
  RTC.begin();
  RTCTime startTime(1, Month::JANUARY, 2023, 22, 12, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_INACTIVE);
  RTC.setTime(startTime);
  
  // Initialize sensors and display
  sensors.begin();
  am2302.begin();
  display.begin();
  display.display();
  
  // Initialize input pins
  initialize_input_pin(PIN_WATER_LVL_1);
  initialize_input_pin(PIN_WATER_LVL_2);
  initialize_input_pin(PIN_PH_METER);
  initialize_input_pin(PIN_EC_READ);
  initialize_input_pin(PIN_AIR_TEMP);
  initialize_input_pin(PIN_WATER_TEMP);
  
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
  
  // Control lights based on current hour
  control_lights(currentHour);
  
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
  if (ph > PH_MAX) {
    
    adjust_PH();
    
    return;
  }
  
  // Check if PPM needs adjustment
  if (ppm < PPM_MIN) {
    
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
  float EC = 1000 / R_Water * K;
  
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
  while (get_PH() > PH_MIN && number_loops < MAX_ADJUSTMENT_LOOPS) {
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
  while (ppm < PPM_MAX && number_loops < MAX_ADJUSTMENT_LOOPS) {
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

void control_lights(int currentHour) {
  // Turn off lights if current hour is outside the range
  if ((currentHour < LIGHTS_ON_HOUR || currentHour > LIGHTS_OFF_HOUR) && light_state == 1) {
    digitalWrite(PIN_LIGHTS, LOW);
    light_state = 0;
  } 
  // Turn on lights if current hour is within the range
  else if ((currentHour >= LIGHTS_ON_HOUR && currentHour <= LIGHTS_OFF_HOUR) && light_state == 0) {
    digitalWrite(PIN_LIGHTS, HIGH);
    light_state = 1;
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

