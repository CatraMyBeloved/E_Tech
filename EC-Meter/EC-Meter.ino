void setup() {
  Serial.begin(9600);
  pinMode(4, OUTPUT);
  pinMode(A0, INPUT);
  digitalWrite(4, LOW);
}
int analog_val = 0;
float V_measurement = 0.0;
float V_R1 = 0.0;
float V_Water = 0.0;
float R_Water = 0.0;
float EC = 0.0;
float ppm = 0.0;
void loop() {
  Serial.println("Measuring...");
  digitalWrite(4, HIGH);
  delay(10),
  analog_val = analogRead(A0);
  delay(10);
  digitalWrite(4, LOW);
  V_Water = analog_val * 5.0/ 1024;
  V_R1 = 5.0 - V_Water;
  R_Water = (1000*V_Water)/(5.0 - V_Water);

  EC = 1000/R_Water*;
  ppm = EC*700;
  Serial.print("Spannung R1: ");
  Serial.println(V_R1);
  Serial.print("Spannung Wasser: ");
  Serial.println(V_Water);
  Serial.print("Wiederstand Wasser: ");
  Serial.println(R_Water);
  Serial.print("EC: ");
  Serial.println(EC);
  Serial.print("PPM: ");
  Serial.println(ppm);

  delay(10000);

}
