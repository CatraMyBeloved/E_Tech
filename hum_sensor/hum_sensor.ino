// Variable für die Speicherung des des Messwertes
int Messwert = 0;

void setup()
{
  // Seriellen Monitor starten
  Serial.begin(9600);
}

void loop() 
{
  // Messwert am analogen Anschluss A0 lesen
  Messwert = analogRead(A0);

  // Messwert im Seriellen Monitor anzeigen
  Serial.print("Feuchtigkeits-Messwert:");
  Serial.println(Messwert);

  // Pause 500 Millisekunden
  delay(500);
}