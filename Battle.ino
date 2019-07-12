/**
 * Hackathon Zumo Robot code.
 * Strategy 2
 * 
 * yoven ayassamy
 * kavi ramsamy
 * yogesh sharma
 * hemanta devi huril
 */

#include <Wire.h>
#include <Zumo32U4.h>

/**********************************************************************
 *                        Zumo library objects                              
 **********************************************************************/
Zumo32U4LCD lcd;
Zumo32U4ButtonA buttonA;
Zumo32U4Buzzer buzzer;
Zumo32U4Motors motors;
Zumo32U4LineSensors lineSensors;
Zumo32U4ProximitySensors proxSensors;

/**********************************************************************
 *                         Line Sensor Variables                              
 **********************************************************************/

// values read by line sensor are stored in this array
unsigned int lineSensorValues[3];

// Threshold difference between the value that the line sensor
// returns for white and black surfaces
// *** To be checked because of different lighting conditions ***
const uint16_t lineSensorThreshold = 1000;

/**********************************************************************
 *                       Proximity Sensor Variables                              
 **********************************************************************/

// The minimum amount of time to spend scanning for nearby opponents, in milliseconds.
const uint16_t scanTimeMin = 100;

// The maximum amount of time to spend scanning for nearby opponents, in milliseconds.
const uint16_t scanTimeMax = 400;

/**********************************************************************
 *                     Motors Sensor Variables                              
 **********************************************************************/

// Speed that the robot uses when backing up.
const uint16_t reverseSpeed = 200;

// The amount of time to spend backing up after detecting a border, in milliseconds.
const uint16_t reverseTime = 200;

// Speed that the robot uses when turning.
const uint16_t turnSpeed = 250;

// Speed the robot uses to move forward at a low speed
// to look for opponents.
const uint16_t forwardSpeed = 200;

// Speed the robot uses to push opponent out of the ring.
const uint16_t rammingSpeed = 400;

// Enum to describe the different state that the robot can be in.
enum State
{
  StateScanning,
  StateDriving,
  StateBacking,
  StateHardTurn
};

// Set the intial state to StateInitial when the robot is just started.
State state = StateDriving;

// Enum to describe the direction that the robot can go.
enum Direction
{
  DirectionLeft,
  DirectionRight,
};

// scanDir is the direction the robot should turn the next time
// it scans for an opponent.
Direction scanDir = DirectionLeft;

Direction hardTurn;

// The time, in milliseconds, that we entered the current top-level state.
uint16_t stateStartTime;

// variable to check if the robot was just started
bool justStarted = true;
int previous = 0;

bool opponentFound = false;

/**********************************************************************
 *                              Set Up                             
 **********************************************************************/
void setup()
{
  // Stop the motors
  motors.setSpeeds(0, 0);

  // Activate the 3 line sensors.
  lineSensors.initThreeSensors();

  // Activate the 3 proximity sensors.
  proxSensors.initThreeSensors();

  // Ask user to press button A to start the robot,
  // then clear the display after 5 seconds,
  // indicating that the robot has started running.
  lcd.clear();
  lcd.gotoXY(0, 0);
  lcd.print("Press A");
  buttonA.waitForPress();
  lcd.clear();
  lcd.print("Started");
  delay(5000);
  previous = millis();
}

/**********************************************************************
 *                              Loop                                 
 **********************************************************************/
void loop()
{
  if (state == StateDriving)
  {
    // This block is executed only once when the robot is just started.
    if (justStarted)
    {
      motors.setSpeeds(200, -200);
      if (millis() - previous > 100)
      {
        justStarted = false;
        motors.setSpeeds(forwardSpeed, forwardSpeed);
      }
    }

    checkBorder();

    proxSensors.read();
    
    if (proxSensors.countsFrontWithLeftLeds() >= 3 || proxSensors.countsFrontWithRightLeds() >= 3) {
      opponentFound = true;
    }

    if (proxSensors.countsRightWithRightLeds() >= 5) {
      hardTurn = DirectionRight;
      changeState(StateHardTurn);
    }

    if (proxSensors.countsLeftWithLeftLeds() >= 5) {
      hardTurn = DirectionLeft;
      changeState(StateHardTurn);
    }

    if (opponentFound) {

      ledRed(1);

      if (proxSensors.countsFrontWithLeftLeds() > proxSensors.countsFrontWithRightLeds()) {
        ledYellow(0);
        motors.setSpeeds(350, rammingSpeed);
      } else if (proxSensors.countsFrontWithLeftLeds() < proxSensors.countsFrontWithRightLeds()) {
        ledYellow(0);
        motors.setSpeeds(rammingSpeed, 350);
      } else {
        ledYellow(1);
        motors.setSpeeds(rammingSpeed, rammingSpeed);
      }

    } else {

      ledRed(0);

      if (proxSensors.countsFrontWithLeftLeds() > proxSensors.countsFrontWithRightLeds()) {
        ledYellow(0);
        motors.setSpeeds(180, forwardSpeed);
      } else if (proxSensors.countsFrontWithLeftLeds() < proxSensors.countsFrontWithRightLeds()) {
        ledYellow(0);
        motors.setSpeeds(forwardSpeed, 180);
      } else {
        ledYellow(0);
        motors.setSpeeds(forwardSpeed, forwardSpeed);
      }
    }
  }
  else if (state == StateBacking)
  {

    motors.setSpeeds(-reverseSpeed, -reverseSpeed);

    // After backing up for a specific amount of time, start scanning.
    if (timeInThisState() >= reverseTime)
    {
      changeState(StateScanning);
    }
  }
  else if (state == StateScanning) // In this state, the robot rotates in place to look for opponents.
  {

    if (scanDir == DirectionRight)
    {
      motors.setSpeeds(turnSpeed, -turnSpeed);
    }
    else
    {
      motors.setSpeeds(-turnSpeed, turnSpeed);
    }

    uint16_t time = timeInThisState();

    if (time > scanTimeMax)
    {
      changeState(StateDriving);
    }
    else if (time > scanTimeMin)
    {
      proxSensors.read();
      if (proxSensors.countsFrontWithLeftLeds() >= 2 || proxSensors.countsFrontWithRightLeds() >= 2)
      {
        changeState(StateDriving);
      }
    }
  }
  else if (state == StateHardTurn) {

    if (hardTurn == DirectionLeft) {
      motors.setSpeeds(-300, 300);
    }
    else {
      motors.setSpeeds(300, -300);
    }

    if (millis() - stateStartTime > 150) {
      opponentFound = true;
      changeState(StateDriving);
    }
  }
}

// Gets the amount of time we have been in this state, in
// milliseconds.  After 65535 milliseconds (65 seconds), this
// overflows to 0.
uint16_t timeInThisState()
{
  return (uint16_t)(millis() - stateStartTime);
}

// Changes to a new state.  It also clears the LCD and turns off
// the LEDs so that the things the previous state were doing do
// not affect the feedback the user sees in the new state.
void changeState(uint8_t newState)
{
  state = (State)newState;
  stateStartTime = millis();
}


void checkBorder() {
   // Check for borders with the line sensors.
    lineSensors.read(lineSensorValues);

    if (lineSensorValues[0] < lineSensorThreshold)
    {
      scanDir = DirectionRight;
      opponentFound = false;
      changeState(StateBacking);
    }

    if (lineSensorValues[2] < lineSensorThreshold)
    {
      scanDir = DirectionLeft;
      opponentFound = false;
      changeState(StateBacking);
    }
}