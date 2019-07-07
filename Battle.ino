#include <Wire.h>
#include <Zumo32U4.h>


Zumo32U4LCD lcd;
Zumo32U4ProximitySensors proximitySensor;

void setup()
{
  // Left, Front and Right init
  proximitySensor.initThreeSensors();
}

void loop()
{
  printReadingsToLCD();
}

void printReadingsToLCD() {

  // Right proximity sensor level
  lcd.gotoXY(0, 0);
  lcd.print(proximitySensor.countsRightWithRightLeds());
  lcd.print(' ');

  // Left proximity sensor level
  lcd.print(proximitySensor.countsLeftWithLeftLeds());
  

  // Front proximity sensor level (using left led)
  lcd.gotoXY(0, 1);
  lcd.print(proximitySensor.countsFrontWithLeftLeds());
  lcd.print(' ');

  // Front proximity sensor level (using right led)
  lcd.print(proximitySensor.countsFrontWithRightLeds());

  delay(10);
}