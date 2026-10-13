#include <Arduino.h>


#define TRIG 13
#define ECHO 12


#define STBY 3
#define motorOne 5
#define motorTwo 6
#define motorThree 7
#define motorFour 8


int baseSpeed     = 115;
int turnDelayRight = 448; //These thresholds had to be adjusted every run to calibrate
int turnDelayLeft  = 467;
int fwdPulseMS    = 160;

float wallDist    = 13.0;
float sideClear   = 20.0;


float getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 20000); //Conversion for distance
  return (duration * 0.0343) / 2.0;
}

float averageDistance() {
  float total = 0;
  int count = 0;

  for (int i = 0; i < 3; i++) {
    float d = getDistance();
    if (d > 0 && d < 200) {
      total += d;
      count++;
    }
    delay(10);
  }
  
  return (count > 0) ? total / count : 999;
}


void STARTMotors(bool on) {
  digitalWrite(STBY, on ? HIGH : LOW);
}

void stopCar() {
  analogWrite(motorOne, 0);
  analogWrite(motorTwo, 0);
  STARTMotors(false);
}

void moveForward(int speed) {
  STARTMotors(true);
  digitalWrite(motorThree, HIGH);
  digitalWrite(motorFour, HIGH);
  analogWrite(motorOne, speed);
  analogWrite(motorTwo, speed);
}

void moveBackward(int speed) {
  STARTMotors(true);
  digitalWrite(motorThree, LOW);
  digitalWrite(motorFour, LOW);
  analogWrite(motorOne, speed);
  analogWrite(motorTwo, speed);
}

void turnRight90(int speed, int delayMS) {
  stopCar();
  delay(150);

  STARTMotors(true);
  digitalWrite(motorThree, LOW); 
  digitalWrite(motorFour, HIGH); 
  analogWrite(motorOne, speed);
  analogWrite(motorTwo, speed);

  delay(delayMS);
  stopCar();
  delay(200);
}

void turnLeft90(int speed, int delayMS) {
  stopCar();
  delay(150);

  STARTMotors(true);
  digitalWrite(motorThree, HIGH); 
  digitalWrite(motorFour, LOW);  
  analogWrite(motorOne, speed);
  analogWrite(motorTwo, speed);

  delay(delayMS);
  stopCar();
  delay(200);
}

void turnAround() {
  turnLeft90(baseSpeed, turnDelayRight);//logic for uturn is two left turns
  delay(250);
}



void setup() { //Initializing
  Serial.begin(9600);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(STBY, OUTPUT);
  pinMode(motorOne, OUTPUT);
  pinMode(motorTwo, OUTPUT);
  pinMode(motorThree, OUTPUT);
  pinMode(motorFour, OUTPUT);

  stopCar();
}


void loop() {
  float frontDist = averageDistance();

  if (frontDist <= wallDist) {
    stopCar();

    turnLeft90(baseSpeed, turnDelayLeft);
    float leftDist = averageDistance();

    if (leftDist > sideClear) {
      moveForward(baseSpeed);
      delay(fwdPulseMS);
      stopCar();
    } 
    
    else {//when the left side is blocked we try right

      stopCar();
      delay(250);

      turnRight90(baseSpeed, turnDelayRight);
      delay(200);
      turnRight90(baseSpeed, turnDelayRight);

      float rightDist = averageDistance();

      if (rightDist > sideClear) {//if right is open we proceed right
        moveForward(baseSpeed);
        delay(fwdPulseMS);
        stopCar();
      } 
      else {//if nothing woorks take a u-turn
        turnAround();
      }
    }
  } 
  
  else {
    moveForward(baseSpeed);
    delay(fwdPulseMS);
    stopCar();
  }

  delay(100);
}
