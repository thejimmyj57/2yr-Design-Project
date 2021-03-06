/* HELLO
  MSE 2202 test code 1
  Language: Arduino
  Authors: Drew, Brooke, Gabriel and Brad
  Date: 19/3/18
  Rev 1 - Initial version
*/

#include <Servo.h>
#include <EEPROM.h>
#include <uSTimer2.h>
#include <CharliePlexM.h>
#include <Wire.h>
#include <I2CEncoder.h>

Servo servo_RightMotor;
Servo servo_LeftMotor;
Servo servo_ArmMotor;
Servo servo_Winch;
Servo servo_Claw;
Servo servo_ClawSwivel;


//port pin constants
const int ci_ContactSwitch = 2;
const int ci_F_Ultrasonic_Ping = 4;   //output plug
const int ci_F_Ultrasonic_Data = 5;   //input plug
const int ci_Winch = 6;
const int ci_Arm_Motor = 7;
const int ci_Right_Motor = 8;
const int ci_Left_Motor = 9;
const int ci_Claw = 10;
const int ci_Claw_Swivel = 11;
const int ci_Mode_Button = 12;
const int ci_LED = 13;
const int ci_S2_Ultrasonic_Ping = A1;   //output plug
const int ci_S2_Ultrasonic_Data = A2;
const int ci_S1_Ultrasonic_Ping = A3;   //output plug
const int ci_S1_Ultrasonic_Data = A4;   //input plug
const int ci_IR1 = A0;
const int ci_IR2 = A5;
const int ci_IR3 = 3;


//constants

// EEPROM addresses

const int ci_Left_Motor_Offset_Address_L = 12;
const int ci_Left_Motor_Offset_Address_H = 13;
const int ci_Right_Motor_Offset_Address_L = 14;
const int ci_Right_Motor_Offset_Address_H = 15;

const int ci_Left_Motor_Stop = 1500;        // 200 for brake mode; 1500 for stop
const int ci_Right_Motor_Stop = 1500;
const int ci_Arm_Servo_Retracted = 55;      //  "
const int ci_Arm_Servo_Extended = 120;      //  "
const int ci_Display_Time = 500;
const int ci_Motor_Calibration_Cycles = 3;
const int ci_Motor_Calibration_Time = 5000;

//variables
byte b_LowByte;
byte b_HighByte;
unsigned long ul_F_Echo_Time;
unsigned int ui_Motors_Speed = 1900;        // Default run speed
unsigned int ui_Left_Motor_Speed = 1500;
unsigned int ui_Right_Motor_Speed = 1500;
long l_Left_Motor_Position;
long l_Right_Motor_Position;
long ul_S1_Echo_Time;
long ul_S2_Echo_Time;
int diff;
int distToSide1;
int distToSide2;
int distToFront;
int findStep = 0;
int searchMode = 0;

int turns = 0;

unsigned long ul_3_Second_timer = 0;
unsigned long ul_Display_Time;
unsigned long ul_Calibration_Time;
unsigned long ui_Left_Motor_Offset;
unsigned long ui_Right_Motor_Offset;

unsigned int ui_Cal_Count;
unsigned int ui_Cal_Cycle;

unsigned int  ui_Robot_State_Index = 0;
//0123456789ABCDEF
unsigned int  ui_Mode_Indicator[6] = {
  0x00,    //B0000000000000000,  //Stop
  0x00FF  //B0000000011111111,  //Run
};

unsigned int  ui_Mode_Indicator_Index = 0;

//display Bits 0,1,2,3, 4, 5, 6,  7,  8,  9,  10,  11,  12,  13,   14,   15
int  iArray[16] = {
  1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 65536
};
int  iArrayIndex = 0;

boolean bt_Heartbeat = true;
boolean bt_3_S_Time_Up = false;
boolean bt_Do_Once = false;
boolean bt_Cal_Initialized = false;

////////////// BUTTON TOGGLE VARIABLES //////////////////////////////////////////////////////////////////////////////
bool ON = false;
unsigned int onTimer = 0;
unsigned int onTimerDelay = 1000;
int MODE = 0 ;
////////////// HEARTBEAT TIMER VARIABLES/////////////////////////////////////////////////////////////////////////////////////
unsigned int heartbeatTimer = 0;
int heartbeatDelay = 0;
////////////// ACTION TIMER VARIABLES/////////////////////////////////////////////////////////////////////////////////////
unsigned int actionTimer = 0;
int actionDelay = 0;
double CSmillis;
bool switchTripped = false;
int Nturns = 0;
double timeUntilTurn = 0;
double sweepTimer = 0;
double gabeTimer = 0;
///////////// ALIGNMENT VARIABLES ////////////////////////////////////////////////////////////////////////
int alignTolerance = 1;
int spinTolerance = 3;
int distFromFrontWall = 15;
int distFromSideWall = 11;
int lastAction = 0; // 0 = straight, 1 = right, 2 = left
int sideDelta = 0;
///////////// SERVO VARIABLES ///////////////////////////////////////
int ClawOpen = 140;
int ClawClosed = 0;
int ClawSwivelOpen = 175;
int ClawSwivelClosed = 45;
bool ArmUp = true;
bool WinchUp = true;
bool s1, s2, s3;
bool hasSeen1 = false;
bool hasSeen3 = false;

void GrabCube();

void setup() {
  Wire.begin();        // Wire library required for I2CEncoder library
  Serial.begin(9600);

  // set up side ultrasonic
  pinMode(ci_S1_Ultrasonic_Ping, OUTPUT);
  pinMode(ci_S1_Ultrasonic_Data, INPUT);

  pinMode(ci_S2_Ultrasonic_Ping, OUTPUT);
  pinMode(ci_S2_Ultrasonic_Data, INPUT);

  //set up front ultrasonic
  pinMode(ci_F_Ultrasonic_Ping, OUTPUT);
  pinMode(ci_F_Ultrasonic_Data, INPUT);

  // set up drive motors
  pinMode(ci_Right_Motor, OUTPUT);
  servo_RightMotor.attach(ci_Right_Motor);
  pinMode(ci_Left_Motor, OUTPUT);
  servo_LeftMotor.attach(ci_Left_Motor);

  // set up arm motors
  pinMode(ci_Arm_Motor, OUTPUT);
  servo_ArmMotor.attach(ci_Arm_Motor);
  pinMode(ci_Winch, OUTPUT);
  servo_Winch.attach(ci_Winch);
  pinMode(ci_Claw, OUTPUT);
  servo_Claw.attach(ci_Claw);
  pinMode(ci_Claw_Swivel, OUTPUT);
  servo_ClawSwivel.attach(ci_Claw_Swivel);

  // setup button
  pinMode(ci_Mode_Button, INPUT_PULLUP);
  pinMode(ci_LED, OUTPUT);

  //setup contact switch
  pinMode(ci_ContactSwitch, INPUT_PULLUP);

  // read saved values from EEPROM
  b_LowByte = EEPROM.read(ci_Left_Motor_Offset_Address_L);
  b_HighByte = EEPROM.read(ci_Left_Motor_Offset_Address_H);
  ui_Left_Motor_Offset = word(b_HighByte, b_LowByte);
  b_LowByte = EEPROM.read(ci_Right_Motor_Offset_Address_L);
  b_HighByte = EEPROM.read(ci_Right_Motor_Offset_Address_H);
  ui_Right_Motor_Offset = word(b_HighByte, b_LowByte);

  //digitalWrite(ci_LED,HIGH);
  ui_Left_Motor_Speed = 1650;
  ui_Right_Motor_Speed = 1650;

  clawUp(true);
  clawSwivelUp(true);
  delay(1000);

}

void acquirePyramid();

void loop()
{
  IRRead();
  if (s1 == true) {
    Serial.print("SENSOR1");
    hasSeen1 = true;
  }
  if (s3 == true) {
    Serial.print("SENSOR1");
    hasSeen3 = true;
  }
  digitalWrite(13, HIGH);
  switch (MODE) {
    case 0:                 //mode to find wall
      {
        Ping();
        if ((distToFront < 25) || (distToSide1 < 25)) //check to see if the side or front are close to a wall
        {
          halt();
          MODE = 1;
        }
        else          //drive forward until the robot is close to a wall
        {
          driveStraight();
        }
        break;
      }
    case 1:               //mode to align robots side with wall
      {
        Ping();              //get new distance values
        spinLeft(130);
        while (!inTolerance(distToSide1, distToSide2))
        {
          Ping();
        }
        halt();
        delay(500);
        MODE = 2;
        /*if ((distToSide2 < 15) && (distToSide1 < 15))    //spin left until the robot aligns itself with the wall
          {
          halt();
          delay(1000);
          MODE = 2;
          }*/
        break;
      }
    case 2:                              // follow wall
      {
        Ping();
        if (distToFront < 20)     //else if the robot needs to turn because a wall is close ahead
        {
          halt();
          delay(200);
          Ping();
          while (distToFront < 15)
          {
            Ping();
            reverse();
            writeMotor();
          }
          MODE = 3;
        }
        else if (diff < -alignTolerance)         //see if back sensor is too far from wall and readjust
        {
          driveLeft();
        }
        else if (diff > alignTolerance)
        {
          driveRight();
        }
        else if ((diff > -2) && (diff < 2))         //if difference is within tolerance
        {
          driveStraight();
        }
        break;
      }
    case 3:                                 //begin turning robot
      {
        turn();
        MODE = 2;
        break;
      }
    case 4:                                                                         //if cube has tripped contact switch
      {
        GrabCube();
        MODE = 9;
        break;
      }
    case 5:                                                                           //pyramid finding
      {
        IRRead();
        break;
      }
    case 6:
      {
        halt();
        acquirePyramid();
        break;
      }

    case 7:                              // follow wall
      {
        alignTolerance = 4; // reducing tolerance
        Ping();
        if (distToFront < (30 + (30 * (turns / 4)))) //else if the robot needs to turn because a wall is close ahead
        {
          halt();
          delay(200);
          //Ping();
          //while (distToFront < 15)
          //{
          // Ping();
          //reverse();
          //writeMotor();
          //}
          MODE = 8;
        }
        else if (diff < -alignTolerance)         //see if back sensor is too far from wall and readjust
        {
          driveLeft();
        }
        else if (diff > alignTolerance)
        {
          driveRight();
        }
        else if ((diff > -2) && (diff < 2))         //if difference is within tolerance
        {
          driveStraight();
        }
        break;
      }
    case 8:                                 //begin turning robot
      {
        turn();
        MODE = 7;
        turns++;
        break;
      }
    case 9:                                 //sweep
      {
        ui_Left_Motor_Speed = 1625;
        ui_Right_Motor_Speed = 1375;
        servo_LeftMotor.writeMicroseconds(1625);
        servo_RightMotor.writeMicroseconds(1375);

        IRRead();
        gabeTimer++;
        if (gabeTimer > 2000) {
          MODE = 10;
          gabeTimer = 0;
        }
        if (s2 == true) {
          MODE = 11;
          gabeTimer = 0;
        }
        // Serial.print(gabeTimer);
        break;

      }
    case 10:                                 //sweep
      {
        ui_Left_Motor_Speed = 1720;
        ui_Right_Motor_Speed = 1720;
        servo_LeftMotor.writeMicroseconds(1720);
        servo_RightMotor.writeMicroseconds(1720);
        gabeTimer++;

        Ping();
        if ((distToFront < 30) || (distToSide1 < 30)) {
          ui_Left_Motor_Speed = 1720;
          ui_Right_Motor_Speed = 1280;
          servo_LeftMotor.writeMicroseconds(1720);
          servo_RightMotor.writeMicroseconds(1280);
          gabeTimer = 0;
          Serial.print("TURNING BITCH FACE ");
        }

        if (gabeTimer > 300) {
          MODE = 9;
          gabeTimer = 0;
        }
        IRRead();
        if (s2 == true) {
          MODE = 11;
          gabeTimer = 0;
        }
        //Serial.print(gabeTimer);
        break;
      }

    case 11:                                 //sweep
      {
        /*servo_LeftMotor.writeMicroseconds(1720);
          servo_RightMotor.writeMicroseconds(1280);
          delay(100);
          servo_LeftMotor.writeMicroseconds(1280);
          servo_RightMotor.writeMicroseconds(1720);
          delay(100);
        */



        IRRead();
        if (s2 == false) {

          ui_Left_Motor_Speed = 1500;
          ui_Right_Motor_Speed = 1500;
          servo_LeftMotor.writeMicroseconds(1500);
          servo_RightMotor.writeMicroseconds(1500);

          gabeTimer++;
          if (gabeTimer > 600) {
            ui_Left_Motor_Speed = 1400;
            ui_Right_Motor_Speed = 1600;
            servo_LeftMotor.writeMicroseconds(1400);
            servo_RightMotor.writeMicroseconds(1600);
          }
          if (gabeTimer > 1000) {
            gabeTimer = 0;
            MODE = 9;
          }
        }
        else
        {
          ui_Left_Motor_Speed = 1625;
          ui_Right_Motor_Speed = 1625;
          servo_LeftMotor.writeMicroseconds(1625);
          servo_RightMotor.writeMicroseconds(1625);
        }

        if ((hasSeen1 == true) || (hasSeen3 == true)) {
          MODE = 12;
        }
        /*if (s3 == true) {
          MODE = 12;
          }*/
        break;
      }

    case 12 : {
        ui_Left_Motor_Speed = 1500;
        ui_Right_Motor_Speed = 1500;
        servo_LeftMotor.writeMicroseconds(1500);
        servo_RightMotor.writeMicroseconds(1500);
        break;
      }

  }







  servo_LeftMotor.writeMicroseconds(ui_Left_Motor_Speed);
  servo_RightMotor.writeMicroseconds(ui_Right_Motor_Speed);

  if (!switchTripped)
  {
    cubeDetection();
  }

  Serial.println(MODE);
}

void cubeDetection()
{
  if (digitalRead(2) == HIGH)
  {
    CSmillis = millis();
  }
  if ((millis() - CSmillis) > 200)
  {
    halt();
    switchTripped = true;
    MODE = 4;
  }
}

void GrabCube()
{
  servo_Claw.write(ClawClosed);
  servo_LeftMotor.writeMicroseconds(1500);
  servo_RightMotor.writeMicroseconds(1500);
  delay(2000);
  servo_ClawSwivel.write(90);                               //set claw swivel to be in jousting position
  delay (2000);
  Serial.print("DONE");
}

void writeMotor()
{
  servo_LeftMotor.writeMicroseconds(ui_Left_Motor_Speed);
  servo_RightMotor.writeMicroseconds(ui_Right_Motor_Speed);
}

void followWall()
{
  if (diff < -alignTolerance)         //see if back sensor is too far from wall and readjust
  {
    driveLeft();
  }
  else if (diff > alignTolerance)
  {
    driveRight();
  }
  else if ((diff > -2) && (diff < 2))         //if difference is within tolerance
  {
    driveStraight();
  }
}

void turn()
{
  ui_Left_Motor_Speed = 1330;
  ui_Right_Motor_Speed = 1670;
  writeMotor();
  delay(600);                                                          //delay so robot can begin turning before considering whether it is aligned with wall after turn
  Ping();              //get new distance values
  while (!inTolerance(distToSide1, distToSide2))
  {
    Ping();
  }
  halt();
  delay(200);
  Ping();
  while (distToSide1 > (distToSide2 + 2))
  {
    Ping();
    spinRight(120);
    writeMotor();
  }
  Nturns++;
}

void reverse()
{
  ui_Left_Motor_Speed = 1380;
  ui_Right_Motor_Speed = 1380;
  writeMotor();
}

void driveStraight()
{
  lastAction = 0;
  ui_Left_Motor_Speed = 1720;
  ui_Right_Motor_Speed = 1720;
  writeMotor();
}

void driveRight()
{
  lastAction = 1;
  ui_Left_Motor_Speed = 1730;
  ui_Right_Motor_Speed = 1660;
  writeMotor();
}

void driveLeft()
{
  lastAction = 2;
  ui_Left_Motor_Speed = 1660;
  ui_Right_Motor_Speed = 1730;
  writeMotor();
}

void spinRight(int speed)
{
  ui_Left_Motor_Speed = (1500 + speed);
  ui_Right_Motor_Speed = (1500 - speed);
  writeMotor();
}

void spinLeft(int speed)
{
  ui_Left_Motor_Speed = (1500 - speed);
  ui_Right_Motor_Speed = (1500 + speed);
  writeMotor();
}

void halt()
{
  ui_Left_Motor_Speed = 1500;
  ui_Right_Motor_Speed = 1500;
  writeMotor();
}

bool inTolerance(int num1, int num2)
{
  if (abs(num1 - num2) < 2 * alignTolerance)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void Ping()
{
  // front side

  digitalWrite(ci_S1_Ultrasonic_Ping, HIGH);
  delayMicroseconds(10);  //The 10 microsecond pause where the pulse in "high"
  digitalWrite(ci_S1_Ultrasonic_Ping, LOW);
  ul_S1_Echo_Time = pulseIn(ci_S1_Ultrasonic_Data, HIGH, 10000);

  // back side

  digitalWrite(ci_S2_Ultrasonic_Ping, HIGH);
  delayMicroseconds(10);  //The 10 microsecond pause where the pulse in "high"
  digitalWrite(ci_S2_Ultrasonic_Ping, LOW);
  ul_S2_Echo_Time = pulseIn(ci_S2_Ultrasonic_Data, HIGH, 10000);

  digitalWrite(ci_F_Ultrasonic_Ping, HIGH);
  delayMicroseconds(10);  //The 10 microsecond pause where the pulse in "high"
  digitalWrite(ci_F_Ultrasonic_Ping, LOW);
  ul_F_Echo_Time = pulseIn(ci_F_Ultrasonic_Data, HIGH, 10000);

  distToSide1 = ul_S1_Echo_Time / 58;
  if (distToSide1 == 0)
  {
    distToSide1 = 9999;
  }
  distToSide2 = ul_S2_Echo_Time / 58;
  if (distToSide2 == 0)
  {
    distToSide2 = 9999;
  }
  distToFront = ul_F_Echo_Time / 58;
  if (distToFront == 0)
  {
    distToFront = 9999;
  }
  diff = distToSide1 - distToSide2; //0 = parallel, +pos = back is closer, -neg = front is closer

  // Print Sensor Readings
  //#ifdef DEBUG_ULTRASONIC
 /* if (MODE == 2) {

    Serial.print("S1Time (microseconds): ");
    Serial.print(ul_S1_Echo_Time, DEC);
    Serial.print(", cm: ");
    Serial.println(ul_S1_Echo_Time / 58); //divide time by 58 to get distance in cm

    Serial.print("S2Time (microseconds): ");
    Serial.print(ul_S2_Echo_Time, DEC);
    Serial.print(", cm: ");
    Serial.println(ul_S2_Echo_Time / 58); //divide time by 58 to get distance in cm

    Serial.print("F()Time (microseconds): ");
    Serial.print(ul_F_Echo_Time, DEC);
    Serial.print(", cm: ");
    Serial.println(ul_F_Echo_Time / 58); //divide time by 58 to get distance in cm
  }*/
  //#endif

}

void rampUp(bool Up)
{
  if (Up == true)
  {
    if (WinchUp == false)
    {
      servo_Winch.writeMicroseconds(1600);
      delay(400);
      servo_Winch.writeMicroseconds(1500);
      WinchUp = true;
    }
  }
  else
  {
    if (WinchUp == true)
    {
      servo_Winch.writeMicroseconds(1400);
      delay(400);
      servo_Winch.writeMicroseconds(1500);
      WinchUp = false;
    }
  }
}

void clawUp(bool Up)
{
  if (Up == true)
  {
    servo_Claw.write(ClawClosed);
  }
  else
  {
    servo_Claw.write(ClawOpen);
  }
}

void clawSwivelUp(bool Up)
{
  if (Up == true)
  {
    servo_ClawSwivel.write(ClawSwivelClosed);
  }
  else
  {
    servo_ClawSwivel.write(ClawSwivelOpen);
  }
}

void armUp(bool Up)
{
  if (Up == true)
  {
    if (ArmUp == false)
    {
      servo_ArmMotor.writeMicroseconds(1300);
      delay(2800);
      servo_ArmMotor.writeMicroseconds(1500);
      ArmUp = true;
    }
  }
  else
  {
    if (ArmUp == true)
    {
      servo_ArmMotor.writeMicroseconds(1700);
      delay(2800);
      servo_ArmMotor.writeMicroseconds(1500);
      ArmUp = false;
    }
  }
}

void IRRead()
{

  if (digitalRead(ci_IR1) == HIGH)
  {
    s1 = true;
  }
  else
  {
    s1 = false;
  }
  if (digitalRead(ci_IR2) == HIGH)
  {
    s2 = true;
  }
  else
  {
    s2 = false;
  }
  if (digitalRead(ci_IR3) == HIGH)
  {
    s3 = true;
  }
  else
  {
    s3 = false;
  }
  // IRSensorAction(s1, s2, s3);
}

void IRSensorAction(bool s1, bool s2, bool s3) {
  Serial.println("IRSENSORACTION");
  if (s1 && s2 && s3) {
    MODE = 6;
    findStep = 1;
  }
  else if (s1 && s2 && !s3) {
    MODE = 6;
    findStep = 2;
  }
  else if (s1 && !s2 && s3) {
    MODE = 6;
    findStep = 3;
  }
  else if (!s1 && s2 && s3) {
    MODE = 6;
    findStep = 4;
  }
  else if (!s1 && !s2 && s3) {
    spinLeft(80);
    findStep = 5;
  }
  else if (s1 && !s2 && !s3) {
    spinRight(80);
    findStep = 6;
  }
  else if (!s1 && s2 && !s3) {                                            // if only the middle sensor sees it, aqcuire pyramid
    driveStraight();
    findStep = 7;
  }
  else if (!s1 && !s2 && !s3) {
    if (findStep == 5)                 //if left sensor had it but now it lost it
    {
      spinLeft(60);
    }
    else if (findStep == 6)                   //if right sensor saw it but now it lost it
    {
      spinRight(60);
    }
    else if (findStep == 7)                                   //need condition
    {
      driveStraight();
    }
    else if (MODE == 4)                                                                                                   //NEED TO WRITE CONDITION OF WHAT TO DO AFTER CUBE IS COLLECTED AND NO PYRAMID IS SEEN
    {
      Ping();
      while (distToFront > 30)
      {
        followWall();
      }
      turn();
      Nturns = 0;
      halt();
    }
    else
    {
      Trace();
    }
  }
}

void Trace()
{
  if (timeUntilTurn = 0)
  {
    timeUntilTurn = millis();
  }
  Ping();
  if (Nturns < 4)
  {
    if (distToFront < 30)
    {
      halt();
      delay(2000);
      turn();
    }
    else if ((millis() - timeUntilTurn) > 1500)
    {
      sweep(30);
    }
    else
    {
      followWall();
    }
  }
  else if (Nturns < 8)                                //if pyramid not found after inital sweep of outer course
  {
    if (distToFront < 60)
    {
      turn();
    }
    else if ((millis() - timeUntilTurn) > 1000)
    {
      sweep(60);
    }
    else
    {
      followWall();
    }

  }
  else {
    if (distToFront < 90)
    {
      turn();
    }
    else if ((millis() - timeUntilTurn) > 750)
    {
      sweep(90);
    }
    else
    {
      followWall();
    }
  }
}

void sweep(int dist)
{
  Ping();
  if (sweepTimer == 0)
  {
    sweepTimer = millis();
  }
  if (((millis() - sweepTimer) > 1500))
  {
    spinRight(90);
    if (distToSide1 < dist)
    {
      if (inTolerance(distToSide1, distToSide2))
      {
        //followWall();
        halt();
        timeUntilTurn = millis();
        sweepTimer = 0;
      }
    }
  }
  else {
    spinLeft(90);
  }

}

void acquirePyramid()                          //when pyramid is right infront
{
  clawUp(true);
  delay(4000);
  clawUp(false);
  delay(4000);
}



