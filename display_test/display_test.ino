

Adafruit_SSD1306 display(128,32,&Wire,-1);

void setup(){
  Serial.begin(9600);
  delay(1000);
  if(display.begin()){
    Serial.println("Display has been connected and initialized.");
  }
  //Shows content of buffer on screen
  display.display();
  delay(5000);
}
void loop(){
  display.clearDisplay();
  display.display();
  delay(2000);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,8);
  display.println(F("test 1 2 3 4 "));
  display.println(F("need water :3"));
  display.display();
  display.startscrollright(0x00, 0x0F);
  delay(10000);
  display.stopscroll();
}