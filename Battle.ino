/**
 * Hackathon Zumo Robot code.
 * Strategy 1
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
const uint16_t scanTimeMin = 200;

// The maximum amount of time to spend scanning for nearby opponents, in milliseconds.
const uint16_t scanTimeMax = 600;


/**********************************************************************
 *                     Motors Sensor Variables                              
 **********************************************************************/

// Speed that the robot uses when backing up.
const uint16_t reverseSpeed = 200;

// Speed that the robot uses when turning.
const uint16_t turnSpeed = 200;

// Speed the robot uses to move forward at a low speed
// to look for opponents.
const uint16_t forwardSpeed = 200;

const uint16_t attackSpeed = 400;

// These two variables specify the speeds to apply to the motors
// when veering left or veering right.  While the robot is
// driving forward, it uses its proximity sensors to scan for
// objects ahead of it and tries to veer towards them.
const uint16_t veerSpeedLow = 0;
const uint16_t veerSpeedHigh = 250;

// Speed the robot uses to push opponent out of the ring.
const uint16_t rammingSpeed = 400;

// The amount of time to spend backing up after detecting a border, in milliseconds.
const uint16_t reverseTime = 200;

// Initial Scanning speed
// This is the speed of the motors when in the StateInitial.
const uint16_t initialScanSpeed = 200;

/**********************************************************************
 *                      Other Variables                              
 **********************************************************************/

// The amount of time to wait between detecting a button press
// and actually starting to move, in milliseconds.  Typical robot
// sumo rules require 5 seconds of waiting.
const uint16_t waitTime = 5000;

// If the robot has been driving forward for this amount of time,
// in milliseconds, without reaching a border, the robot decides
// that it must be pushing on another robot and this is a
// stalemate, so it increases its motor speed.
const uint16_t stalemateTime = 4000;

// This variable is used together with millis()
// to make the robot oscillate to scan for opponents.
int scanningPrevTime = 0;

// This variable is used to store the direction
// at which the robot should turn when oscillating
// in the InitialState to look for opponent.
bool toTurnRight = true;

// This variable determines if the robot has just been switched on.
bool justStarted = true;

// Enum to describe the different state that the robot can be in.
enum State
{
  StateScanning,
  StateDriving,
  StateBacking,
  StateInitial
};

// Set the intial state to StateInitial when the robot is just started.
State state = StateInitial;

// Enum to describe the direction that the robot can go.
enum Direction
{
  DirectionLeft,
  DirectionRight,
};

// scanDir is the direction the robot should turn the next time
// it scans for an opponent.
Direction scanDir = DirectionLeft;

// The time, in milliseconds, that we entered the current top-level state.
uint16_t stateStartTime;

// The time, in milliseconds, that the LCD was last updated.
uint16_t displayTime;

// This gets set to true whenever we change to a new state.
// A state can read and write this variable this in order to
// perform actions just once at the beginning of the state.
bool justChangedState;

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
  delay(waitTime);
  lcd.clear();
  scanningPrevTime = millis();
}

/**********************************************************************
 *                              Loop                                 
 **********************************************************************/
void loop()
{
  if (state == StateInitial) {  // In this state, the robot oscillates in place to look for opponents.

      if (toTurnRight) {
          motors.setSpeeds(initialScanSpeed, -initialScanSpeed);
      } else {
          motors.setSpeeds(-initialScanSpeed, initialScanSpeed);
      }

      if (justStarted) {

          if (millis() - scanningPrevTime > 250) {
              justStarted = false;
              toTurnRight = !toTurnRight;
              scanningPrevTime = millis();
          }
      } else {
          if (millis() - scanningPrevTime > 500) {
              toTurnRight = !toTurnRight;
              scanningPrevTime = millis();
          }
      }

      // Read the proximity sensors and if anything is detected
      // by the front proximity receiver, move forward.
      proxSensors.read();
      if (proxSensors.countsFrontWithLeftLeds() > 2 || proxSensors.countsFrontWithRightLeds() > 2) {
        changeState(StateDriving);
      }

  } else if (state == StateBacking) { // In this state, the robot drives in reverse.
    
    if (justChangedState) {
      justChangedState = false;
    }

    motors.setSpeeds(-reverseSpeed, -reverseSpeed);

    // After backing up for a specific amount of time, start scanning.
    if (timeInThisState() >= reverseTime) {
      changeState(StateScanning);
    }

  } else if (state == StateScanning) { // In this state, the robot rotates in place to look for opponents.

    if (justChangedState) {
      justChangedState = false;
    }

    if (scanDir == DirectionRight) {
      motors.setSpeeds(turnSpeed, -turnSpeed);
    } else {
      motors.setSpeeds(-turnSpeed, turnSpeed);
    }

    uint16_t time = timeInThisState();

    if (time > scanTimeMax) {
      // We have not seen anything for a while, so start driving.
      changeState(StateDriving);
    } else if (time > scanTimeMin) {

      // Read the proximity sensors.  If we detect anything with
      // the front sensor, then start driving forwards.
      proxSensors.read();
      if (proxSensors.countsFrontWithLeftLeds() >= 2 || proxSensors.countsFrontWithRightLeds() >= 2) {
        changeState(StateDriving);
      }
    }
  } else if (state == StateDriving) {
    /*
     * In this state we drive forward while also looking for the
     * opponent using the proximity sensors and checking for the
     * white border.
     */

    if (justChangedState)
    {
      justChangedState = false;
    }

    // Check for borders.
    lineSensors.read(lineSensorValues);
    if (lineSensorValues[0] < lineSensorThreshold)
    {
      scanDir = DirectionRight;
      changeState(StateBacking);
    }
    if (lineSensorValues[2] < lineSensorThreshold)
    {
      scanDir = DirectionLeft;
      changeState(StateBacking);
    }

    // Read the proximity sensors to see if know where the
    // opponent is.
    proxSensors.read();
    uint8_t sum = proxSensors.countsFrontWithRightLeds() + proxSensors.countsFrontWithLeftLeds();
    int8_t diff = proxSensors.countsFrontWithRightLeds() - proxSensors.countsFrontWithLeftLeds();

    if (sum >= 4 || timeInThisState() > stalemateTime){
      // The front sensor is getting a strong signal, or we have
      // been driving forward for a while now without seeing the
      // border.  Either way, there is probably a robot in front
      // of us and we should switch to ramming speed to try to
      // push the robot out of the ring.
      motors.setSpeeds(rammingSpeed, rammingSpeed);

      // Turn on the red LED when ramming.
      ledRed(1);
    }
    else if (sum > 2){
      // We don't see anything with the front sensor, so just
      // keep driving forward.  Also monitor the side sensors; if
      // they see an object then we want to go to the scanning
      // state and turn torwards that object.

      motors.setSpeeds(forwardSpeed, forwardSpeed);

      if (proxSensors.countsLeftWithLeftLeds() >= 2){
        // Detected something to the left.
        scanDir = DirectionLeft;
        changeState(StateScanning);
      }

      if (proxSensors.countsRightWithRightLeds() >= 2){
        // Detected something to the right.
        scanDir = DirectionRight;
        changeState(StateScanning);
      }
    }
    else{
      // We see something with the front sensor but it is not a
      // strong reading.

      if (diff >= 1){
        // The right-side reading is stronger, so veer to the right.
        motors.setSpeeds(veerSpeedHigh, veerSpeedLow);
      }
      else if (diff <= -1){
        // The left-side reading is stronger, so veer to the left.
        motors.setSpeeds(veerSpeedLow, veerSpeedHigh);
      }
      else{
        // Both readings are equal, so just drive forward.
        motors.setSpeeds(attackSpeed, attackSpeed);
      }

    }
  }
}

// Gets the amount of time we have been in this state, in
// milliseconds.  After 65535 milliseconds (65 seconds), this
// overflows to 0.
uint16_t timeInThisState(){
  return (uint16_t)(millis() - stateStartTime);
}

// Changes to a new state.  It also clears the LCD and turns off
// the LEDs so that the things the previous state were doing do
// not affect the feedback the user sees in the new state.
void changeState(uint8_t newState){
  state = (State)newState;
  justChangedState = true;
  stateStartTime = millis();
}
