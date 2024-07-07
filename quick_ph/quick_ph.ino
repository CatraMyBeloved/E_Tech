#define PIN_PH A3

void setup() {
  Serial.begin(9600);
  delay(5000);
  pinMode(PIN_PH,INPUT);
  Serial.println("Beginning Measurement");
}

void loop() {
  Serial.println(analogRead(PIN_PH)* (5/1024)*3.5 +0.15); 
  delay(2); 
}
