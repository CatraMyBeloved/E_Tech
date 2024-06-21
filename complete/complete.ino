// Include all necessary libraries

// TODO: calibrate K value for EC-meter
#include <RTC.h> // Real Time Clock library
#include <DallasTemperature.h> // Library for water temperature sensor
#include <OneWire.h> // Library for one-wire communication

#include <AM2302-Sensor.h> // Library for air temperature and humidity sensor

#include <Wire.h> // Library for I2C communication
#include <Adafruit_SSD1306.h> // Library for OLED display
#include <Adafruit_GFX.h> // Graphics library for OLED display

// Define pins
// Analog pins
#define water_level_1 A0
#define water_level_2 A1
#define ph_meter A3
#define EC_Read A2
// Digital pins
#define lights 8
#define EC_Power 7
#define air_temp_pin 6
#define water_temp_pin 2
// Pump pins
#define pump_1 3
#define pump_2 4
#define pump_3 5

// Initialize settings
int distance = 30000; // Measurement interval in milliseconds

// pH value range
float ph_max = 6.7;
float ph_min = 6.0;

// PPM value range
float ppm_min = 800;
float ppm_max = 1100;

// Air temperature and humidity sensor
AM2302::AM2302_Sensor am2302(air_temp_pin);

// Water temperature sensor
OneWire oneWire(water_temp_pin);
DallasTemperature sensors(&oneWire);

// OLED display
Adafruit_SSD1306 display(128, 32, &Wire, -1);

// Declare variables for sensor readings
int water_level = 0;
float ph = 0.0;
float temp_water = 0.0;
float temp_air = 0.0;
float hum_air = 0.0;
float ppm = 0.0;

// Constants for EC measurement
float V_Ref = 5.0; // Reference voltage
float VOLTAGE_RESOLUTION = 1024.0; // ADC resolution
float PPM_const = 700.0; // Conversion constant for PPM

int max_loops = 10; // Max number of adjustment loops

// Function declarations
float get_PPM();
float get_PH();
void adjust_PH();
void adjust_PPM();
void initialize_pin(int pin);
void update_display();

void setup() {
  Serial.begin(9600); // Initialize serial communication
  
  // Begin real-time clock, set start time
  RTC.begin();
  RTCTime startTime(1, Month::JANUARY, 2023, 0, 0, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_INACTIVE);
  RTC.setTime(startTime);
  
  // Initialize sensors and display
  sensors.begin();
  am2302.begin();
  display.begin();
  
  // Initialize input pins
  pinMode(water_level_1, INPUT);
  pinMode(water_level_2, INPUT);
  pinMode(ph_meter, INPUT);
  pinMode(EC_Read, INPUT);
  pinMode(air_temp_pin, INPUT);
  pinMode(water_temp_pin, INPUT);
  
  // Initialize output pins
  initialize_pin(pump_1);
  initialize_pin(pump_2);
  initialize_pin(pump_3);
  initialize_pin(lights);
  initialize_pin(EC_Power);

  delay(2000); // Wait for sensors to stabilize
  
  Serial.print("Sensors initialized. Beginning loop. Current settings: measurements taken every ");
  Serial.print(distance / 1000);
  Serial.println(" Seconds");
  
  display.display(); // Display initial message
}

void loop() {
  Serial.print("Measurement starting.");
  
  // Use real-time clock to control lights, get current time
  RTCTime currentTime;
  RTC.getTime(currentTime);
  
  // Get current hour
  int currentHour = currentTime.getHour();
  
  // Turn off lights at night between 8 PM and 7 AM
  if ((currentHour < 7) || (currentHour > 21)) {
    digitalWrite(lights, LOW);
  } else {
    digitalWrite(lights, HIGH);
  }

  // Measure water temperature
  sensors.requestTemperatures();
  temp_water = sensors.getTempCByIndex(0);

  // Measure pH value
  ph = get_PH();

  // Measure air temperature and humidity
  temp_air = am2302.get_Temperature();
  hum_air = am2302.get_Humidity();

  // Measure EC and PPM
  ppm = get_PPM();

  // Check if pH needs adjustment
  if (ph > ph_max) {
    adjust_PH();
  }
  
  // Check if PPM needs adjustment
  if (ppm < ppm_min) {
    adjust_PPM();
  }

  // Update the display with current sensor values
  update_display();

  delay(distance); // Wait for the next measurement cycle
}

void update_display() {
  // Clear the display
  display.clearDisplay();
  
  // Set text size and color
  display.setTextSize(0);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  
  // Print sensor values on the display
  display.print("Water temp: ");
  display.println(temp_water);
  display.print("Air temp: ");
  display.println(temp_air);
  display.print("PH-Value: ");
  display.println(ph);
  display.print("Humidity: ");
  display.println(hum_air);
  display.display(); // Update the display with new data
}

float get_PPM() {
  float K = 2.0; // Calibration constant for EC measurement
  digitalWrite(EC_Power, HIGH); // Power the EC meter
  delay(10); // Wait for stable reading
  
  // Get voltage drop across the water and convert to millivolts
  float V_Water = analogRead(EC_Read) * V_Ref / VOLTAGE_RESOLUTION;
  
  // Turn off power to avoid electrolysis
  digitalWrite(EC_Power, LOW);
  
  // Calculate resistance of water
  float R_Water = (1000 * V_Water) / (V_Ref - V_Water);
  
  // Calculate electrical conductivity in milliSiemens
  float EC = 1000 / R_Water * K;
  
  // Convert EC to PPM
  float PPM = EC * PPM_const;
  
  return PPM;
}

float get_PH() {
  float res = 0.0;
  
  // Measure pH over one period of the 50 Hz pump (20 ms) to average out noise
  for (int i = 0; i < 10; i++) {
    res += analogRead(ph_meter);
    delay(2);
  }
  
  // Convert analog value to pH
  res = (res / 10 * 5.0 / 1024.0 * 3.5) + 0.15;
  
  return res;
}

void adjust_PH() {
  int number_loops = 0; // Initialize loop counter
  
  // Adjust pH until it's within the desired range or max loops reached
  while (get_PH() > ph_min && number_loops < max_loops) {
    // Turn on pH-down pump for 6 seconds
    digitalWrite(pump_2, HIGH);
    delay(6000);
    digitalWrite(pump_2, LOW);
    
    // Wait 30 seconds for pH to adjust
    delay(30000);
    number_loops++;
  }
}

void adjust_PPM() {
  int number_loops = 0; // Initialize loop counter
  
  // Adjust PPM until it's within the desired range or max loops reached
  while (get_PPM() < ppm_max && number_loops < max_loops) {
    // Turn on nutrient pump for 6 seconds
    digitalWrite(pump_3, HIGH);
    delay(6000);
    digitalWrite(pump_3, LOW);
    
    // Wait 30 seconds for nutrients to mix
    delay(30000);
    number_loops++;
  }
}

void initialize_pin(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}
