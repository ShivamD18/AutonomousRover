// Maze-solving car: ultrasonic wall detection + hard-coded turn sequence
// Motor driver: TB6612

// ---------- Pins ----------
#define TRIG   13   // Ultrasonic sensor
#define ECHO   12

#define STBY    3   // TB6612 standby
#define motor1  5   // PWM (speed)
#define motor2  6   // PWM (speed)
#define motor3  7   // Direction
#define motor4  8   // Direction

// ---------- Tuning ----------
const int   baseSpeed      = 115;    // PWM speed (0-255)
const int   turnDelayRight = 448;    // ms for a 90 degree right turn
const int   turnDelayLeft  = 467;    // ms for a 90 degree left turn
const int   fwdPulseMS     = 150;    // ms of driving per forward step
const int   rampDriveMS    = 2000;   // ms to drive after the last turn (onto the ramp)

const float wallDist       = 13.0;   // cm: closer than this counts as a wall
const int   wallConfirm    = 2;      // consecutive wall readings needed before turning

// Hard-coded solution for the given maze
const char turnSeq[] = {'L', 'R', 'L', 'L', 'L', 'R', 'R'};
const int  numTurns  = sizeof(turnSeq) / sizeof(turnSeq[0]);

// ---------- State ----------
int  turnIndex = 0;
int  wallHits  = 0;
bool done      = false;

// ---------- Ultrasonic sensor ----------
// Returns distance in cm, or -1 if no echo was received
float getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  unsigned long duration = pulseIn(ECHO, HIGH, 25000UL);
  if (duration == 0) return -1.0f;           // timeout: no valid echo
  return (duration * 0.0343f) / 2.0f;        // speed of sound conversion
}

// Median of up to 5 valid readings (rejects spikes better than a mean).
// Returns -1 if fewer than 3 readings were valid.
float readFront() {
  float v[5];
  int n = 0;

  for (int i = 0; i < 5; i++) {
    float d = getDistance();
    if (d > 2.0f && d < 200.0f) v[n++] = d;
    delay(10);
  }
  if (n < 3) return -1.0f;

  // insertion sort
  for (int i = 1; i < n; i++) {
    float key = v[i];
    int j = i - 1;
    while (j >= 0 && v[j] > key) { v[j + 1] = v[j]; j--; }
    v[j + 1] = key;
  }
  return v[n / 2];
}

// ---------- Motor control ----------
void startMotors(bool on) { digitalWrite(STBY, on ? HIGH : LOW); }

void stopCar() {
  analogWrite(motor1, 0);
  analogWrite(motor2, 0);
  startMotors(false);
}

void moveForward(int speed) {
  startMotors(true);
  digitalWrite(motor3, HIGH);
  digitalWrite(motor4, HIGH);
  analogWrite(motor1, speed);
  analogWrite(motor2, speed);
}

void forwardPulse() {
  moveForward(baseSpeed);
  delay(fwdPulseMS);
  stopCar();
  delay(80);
}

void turnRight90(int speed, int delayMS) {
  stopCar(); delay(150);
  startMotors(true);
  digitalWrite(motor3, LOW);
  digitalWrite(motor4, HIGH);
  analogWrite(motor1, speed);
  analogWrite(motor2, speed);
  delay(delayMS);
  stopCar(); delay(200);
}

void turnLeft90(int speed, int delayMS) {
  stopCar(); delay(150);
  startMotors(true);
  digitalWrite(motor3, HIGH);
  digitalWrite(motor4, LOW);
  analogWrite(motor1, speed);
  analogWrite(motor2, speed);
  delay(delayMS);
  stopCar(); delay(200);
}

// ---------- Setup (runs once) ----------
void setup() {
  Serial.begin(9600);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(STBY, OUTPUT);
  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);
  pinMode(motor3, OUTPUT);
  pinMode(motor4, OUTPUT);

  stopCar();
}

// ---------- Main loop ----------
void loop() {
  if (done) { stopCar(); return; }

  float front = readFront();
  Serial.print("Front: "); Serial.println(front);

  // No valid reading: never assume the path is clear, stop and retry
  if (front < 0) {
    stopCar();
    Serial.println("Sensor: no valid reading");
    delay(50);
    return;
  }

  // Path is clear: keep moving toward the next wall
  if (front > wallDist) {
    wallHits = 0;
    forwardPulse();
    return;
  }

  // Wall detected: confirm with consecutive readings before turning
  stopCar();
  if (++wallHits < wallConfirm) return;
  wallHits = 0;

  char cmd = turnSeq[turnIndex];
  Serial.print("Turn: "); Serial.println(cmd);

  if (cmd == 'L' || cmd == 'l')      turnLeft90(baseSpeed, turnDelayLeft);
  else if (cmd == 'R' || cmd == 'r') turnRight90(baseSpeed, turnDelayRight);

  turnIndex++;

  // Last turn done: drive out of the maze and onto the ramp, then stop
  if (turnIndex >= numTurns) {
    moveForward(baseSpeed);
    delay(rampDriveMS);
    stopCar();
    done = true;
  }
}