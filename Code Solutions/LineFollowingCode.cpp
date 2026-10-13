#define R_PIN   A0 //All these pins are for line following
#define MR_PIN  A1
#define M_PIN   A2
#define ML_PIN  A3
#define L_PIN   A4

#define STBY 3
#define motorOne 5
#define motorTwo 6
#define motorThree 7
#define motorFour 8


int TH = 400;

float Kp = 22.0;
float Ki = 0.0;
float Kd = 10.0;

int baseSpeed = 60;
int maxSpeed  = 100;

float error = 0, lastError = 0, integral = 0; //error mitigations
float lastDerivative = 0;
float alpha = 0.5;  


void setMotors(int left, int right) { //initialising our motors
  left  = constrain(left, -maxSpeed, maxSpeed);
  right = constrain(right, -maxSpeed, maxSpeed);

  if (left >= 0) {
    digitalWrite(motorThree, HIGH);
    analogWrite(motorOne, left);
  } else {
    digitalWrite(motorThree, LOW);
    analogWrite(motorOne, -left);
  }

  if (right >= 0) {
    digitalWrite(motorFour, HIGH);
    analogWrite(motorTwo, right);
  } else {
    digitalWrite(motorFour, LOW);
    analogWrite(motorTwo, -right);
  }
}

void stopMotors() {
  analogWrite(motorOne, 0);
  analogWrite(motorTwo, 0);
}


void readSensors(bool &L, bool &ML, bool &M, bool &MR, bool &R) {
  R  = (analogRead(R_PIN)  > TH); //direction changes
  MR = (analogRead(MR_PIN) > TH);
  M  = (analogRead(M_PIN)  > TH);
  ML = (analogRead(ML_PIN) > TH);
  L  = (analogRead(L_PIN)  > TH);
}

float getLineError(bool L, bool ML, bool M, bool MR, bool R) {
  int num = (L*2) + (ML*1) + (M*0) + (MR*(-1)) + (R*(-2));
  int den = L + ML + M + MR + R;
  if (den == 0) return 999;
  return (float)num / den;
}


bool TapeonLeft(bool L, bool ML, bool M, bool MR, bool R) { //If we see the black wire on the left side, we turn towards left
  return (L && ML && !M && !MR && !R);
}

bool TapeonRight(bool L, bool ML, bool M, bool MR, bool R) {//If on right side we turn right
  return (R && MR && !M && !ML && !L);
}


void MoveToLeft() {
  stopMotors();

  while (true) {
    setMotors(-50, 50);
    bool L, ML, M, MR, R;
    readSensors(L, ML, M, MR, R);
    if (M || MR) { 
      stopMotors();
      delay(200);
      break;
    }
  }
}

void MoveToRight() {
  stopMotors();//initialize

  while (true) {
    setMotors(50, -50);//opposite of left
    bool L, ML, M, MR, R;
    readSensors(L, ML, M, MR, R);
    if (M || ML) { 
      stopMotors();
      delay(200);
      break;
    }
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(R_PIN, INPUT);
  pinMode(MR_PIN, INPUT);
  pinMode(M_PIN, INPUT);
  pinMode(ML_PIN, INPUT);
  pinMode(L_PIN, INPUT);

  pinMode(motorOne, OUTPUT);
  pinMode(motorTwo, OUTPUT);
  pinMode(motorThree, OUTPUT);
  pinMode(motorFour, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);
}


void loop() {
  bool R, MR, M, ML, L;
  readSensors(L, ML, M, MR, R);

  if (!R && !MR && !M && !ML && !L) { //Detects nothing
    setMotors(0, 0);
    return;
  }
  if (TapeonLeft(L, ML, M, MR, R)) {
    MoveToLeft();
    return;
  }
  if (TapeonRight(L, ML, M, MR, R)) {
    MoveToRight();
    return;
  }

  float pos = getLineError(L, ML, M, MR, R);
  if (pos == 999) {
    setMotors(0, 0);
    return;
  }

  //error mitigation based on lecture
  error = pos;
  integral += error;
  float rawDeriv = error - lastError;
  float derivative = alpha * rawDeriv + (1 - alpha) * lastDerivative;
  lastDerivative = derivative;
  lastError = error;

  float correction = Kp * error + Ki * integral + Kd * derivative;

  float absErr = fabs(error);
  int dynamicBase = baseSpeed - (absErr * 8);
  dynamicBase = constrain(dynamicBase, 40, baseSpeed);

  int leftSpeed  = dynamicBase + correction;
  int rightSpeed = dynamicBase - correction;
  setMotors(leftSpeed, rightSpeed);

  delay(5);
}




