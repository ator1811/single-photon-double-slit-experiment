/*

  versie informatie:
  v1.0: finale versie, gebruikt voor uitrol.
  v1.1: Who-are-you resonse added
  v1.2: debug uitgezet (Rindert Nauta)
        Who are you (x-->X) in home routine gezet.
        s (stop) in scan mode gezet. 



  definitions

  OP BASIS POLULU
  ontwikkeld uit SPCM-stepper-test-leonardo
  Ideetjes/todo
  - Laser aan/uit?
  - versneld scannen ter demonstratie
  - na elke scan terug naar home
  - potmeter verwijderen, oplossen met instellingen offset



  Based on Accelstepper
  http://www.pjrc.com/teensy/td_libs_AccelStepper.html
  v0.1: init

  --0------------+-------------+----------------+----------------------
  --|------------|-------------|----------------|--------------------
  --LimitPos------leftpos-------midpos-----------rightpos----------
  ...............<---maxLeft--><--maxRight------>
  ...<------offSet------------->

  modes:

  3: homing mode, laser uit, detector off
  2: move to left (quick), laser off, detector off
  5: scan mode, laser and detector on
  0: stop and wait, laser and detector off


  LimitPos switch (interrupt)

  SPCM in External Triggered Counter


  stepper.run(): run accelerated to destination
  stepper.runSpeed(): constant speed until eaturnaty
  stepper.runToPosition(): blocks, constantspeed to destination
  stepper.runSpeedToPosistion(): constantspeed to destination

  to be used with v2.0 box

  V2.1  WAIT_TIME


*/
#include "AccelStepper.h"

// interface:
#define DIR_PIN 6
#define STEP_PIN 3
#define PRINT  false //debug printing off

AccelStepper stepper(1, STEP_PIN, DIR_PIN);

int laserPin = 10;   // PWM laser connected to digital pin 10, tmp on 13

int midPos;
int leftPos;
int rightPos;
int stepSize;


//future use
//int inProgressLED = 10;
int scanningPin = 12;
int LimitPosPlus = 4;
int LimitPosMin = 5;



// variables
long maxLeft = 750; //in um
long maxRight = 750; // in um

//long offSet = 4860; //wit afstand van LimitPos tot geschatte middenpositie
//long offSet = 5025; //groen afstand van LimitPos tot geschatte middenpositie
long offSet = 5000; //blauw afstand van LimitPos tot geschatte middenpositie
//long offSet = 5200; //geel afstand van LimitPos tot geschatte middenpositie

// groen: 5025
// blauw: 5000
// geel: 5200
// wit: 4860


float homeSpeed = 700;         //calSpeed
float ijlSpeed = 700;         //fast moving
float scanSpeed = 300;          //scanSpeed

long numberOfBins = 300;      //to be alligned with SPCSM software

int runMode = 4; // start straks met homing
long previousMillis = 0;        // will store last time printfunction was updated

bool LaserPower = LOW;

int WAIT_TIME = 100;


void setup() {
  Serial.begin(9600);     // open the serial connection at 9600bps
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(1000);

  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);

  //  pinMode(resetButton, INPUT);   // reset = input
  pinMode(LimitPosPlus, INPUT);   //autoSwitch = input
  pinMode(LimitPosMin, INPUT);   //autoSwitch = input

  // controles
  pinMode(laserPin, OUTPUT); // set laserpin 10 to output
  pinMode(scanningPin, OUTPUT); // set scanning pin to output


  //switch off laser and scanning
  LaserPower = false;
  digitalWrite(laserPin, LOW); //laser off;
  digitalWrite(scanningPin, LOW);



  stepper.setCurrentPosition(offSet);  //determine midposistion as a reference Voorlopig is leftpos het nulpunt, er vanuit gaande dat laatste keer in leftpos is achtergelaten
}

void loop() {
  // vaststellen van absolute posities
  int midPos = offSet;
  int leftPos = midPos - maxLeft;
  int rightPos = midPos + maxRight;
  int stepSize = (rightPos - leftPos) / numberOfBins; // zorg dat stepSize een geheel getal is!



  if (Serial.available() > 0) // kijk of de PC iets gestuurd heeft
  {
    char c = Serial.read(); // haal de data op, kijk welke toets is ingedrukt:
    if (c == 'l') { //go to left posistion
      runMode = 1;
    }
    if (c == 'm') { //go to mid posistion
      runMode = 2;
    }
    if (c == 'r') { //go to right posistion
      runMode = 3;
    }
    if (c == 'h') { //homing procedure
      runMode = 4;
    }
    if (c == 'g') { //scanning
      runMode = 5;
    }
    if (c == 's') { //stop
      runMode = 0;
    }
    if ((c == 'H') &&  (runMode == 0)){ //laser High
      LaserPower = HIGH;
    }  
    if ((c == 'L') &&  (runMode == 0)) { //laser Low
      LaserPower = LOW;
    }
    if (c == 'x') { //who-are-you?
      Serial.print('X');
    }
    if (c == 'W') {
      WAIT_TIME = Serial.parseInt() ;
      Serial.println(WAIT_TIME);
      if (WAIT_TIME == 0) { WAIT_TIME = 100;}
    }

  }


  // printPos(leftPos, midPos, rightPos, stepSize);

  // ----------- go to leftpos ------------
  if (runMode == 1) {
    gotoPos(HIGH, leftPos);
    if (PRINT)    printLine2();
  }

  // ----------- go to midpos ------------
  if (runMode == 2) {
    gotoPos(HIGH, midPos);
    if (PRINT)    printLine2();
  }


  // ----------- go to rightpos ------------
  if (runMode == 3) {
    gotoPos(HIGH, rightPos);
    if (PRINT)    printLine2();
  }

  // --------homing mode -------------------
  if (runMode == 4) { // homing
    digitalWrite(laserPin, LOW); //laser off;
    digitalWrite(scanningPin, LOW); //scanning of
    stepper.setSpeed(-homeSpeed); //set constant speed
    while (digitalRead(LimitPosMin) == LOW) { // || (digitalRead(LimitPosMin) == LOW)) {
      stepper.runSpeed(); // returns immediately, runs with setSpeed
      delay(2);
       if (Serial.available() > 0) // kijk of de PC iets gestuurd heeft (*Rindert*)
        {
          char c = Serial.read(); // haal de data op, kijk welke toets is ingedrukt:
          if (c == 'x') { //who-are-you?
          Serial.print('X');
          }
    }

      if (PRINT)    printLine2();
    }
    stepper.setCurrentPosition(0); // LimitPos reached, set to 0
    runMode = -1; // reset, go to homing posisition

  }


  // ----------- go to start and start scanning (mode = 5) ------------
  if (runMode == 5) { // go to start and start mode5
    gotoPos(LOW, leftPos);
    if (PRINT)    printLine2();

    stepper.setSpeed(scanSpeed);
    digitalWrite(laserPin, HIGH); //laser on
    for (int next = leftPos; next < rightPos; next = next + stepSize)
    {
      stepper.moveTo(next);
      stepper.setSpeed(scanSpeed);
      while (stepper.currentPosition() != next) {

        stepper.runSpeedToPosition();
      }
      delay(10);
      digitalWrite(scanningPin, HIGH); // start counter
      digitalWrite(scanningPin, LOW); // start counter
      delay(WAIT_TIME);
      digitalWrite(scanningPin, HIGH); // stop counter
      digitalWrite(scanningPin, LOW); // stop counter


      if (PRINT)    printLine2();
      if (Serial.available() > 0) // kijk of de PC iets gestuurd heeft 
        {
          char c = Serial.read(); // haal de data op, kijk welke toets is ingedrukt:
          if (c == 's') { //stop ==> break
            digitalWrite(laserPin, LOW); //laser off
            runMode = -1; // homing
            break;
          }
        }
    }
    digitalWrite(laserPin, LOW); //laser off
    runMode = -1; // homing
  }

  // ----------- wait ------------
  if (runMode == 0) { // wait
    digitalWrite(laserPin, LaserPower); //laser ;
    digitalWrite(scanningPin, LOW);
    if (PRINT)    printLine2();
  }

  // ----------- return to begin and wait------------
  if (runMode == -1) { // go to start and wait
    gotoPos(LOW, leftPos);
    if (PRINT)    printLine2();
    runMode = 0;
  }
}





void printLine2() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > 500) {
    //    int posPerc = map(stepper.currentPosition(),-maxLeft,maxRight,-1000,1000);
    Serial.print(runMode);
    Serial.print("; ");
    Serial.println(stepper.currentPosition());
    previousMillis = currentMillis;
  }
}

void printPos(int left, int mid, int right, int stepp) {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > 500) {
    Serial.print(left);
    Serial.print(" - ");
    Serial.print(mid);
    Serial.print(" - ");
    Serial.print(right);
    Serial.print(" - ");
    Serial.println(stepp);

  }
}

void gotoPos(int laserOn, int Pos) {
  digitalWrite(laserPin, laserOn); //laser off;
  digitalWrite(scanningPin, LOW);
  stepper.moveTo(Pos);
  stepper.setSpeed(ijlSpeed);
  while (stepper.currentPosition() != Pos) {
    stepper.runSpeedToPosition();
  }
}







