#include <EEPROM.h>
#include <SoftwareSerial.h>

#define STEP_PIN 5
#define DIR_PIN  4
#define EN_PIN   2

const int RxPin = 10; 
const int TxPin = 11; // Not used, but required by library
SoftwareSerial espSerial(RxPin, TxPin);

int stepDelay = 2000;           
long currentPosition = 0;       
long targetPosition = 0;        

#define NUM_FLOORS 10
long floorSteps[NUM_FLOORS] = {930, 1800, 2500, 3400, 4150, 4950, 5750, 6550, 7350, 8150};

#define ORIGIN_ADDR 0

void setup() {
  
  Serial.begin(9600);
  Serial.println("--- SYSTEM START ---");
  Serial.println("Waiting for data from ESP32 on Pin 10...");

  espSerial.begin(9600);
  espSerial.setTimeout(50); 

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW); 

  EEPROM.get(ORIGIN_ADDR, currentPosition);
  
  if (currentPosition < 0 || currentPosition > floorSteps[NUM_FLOORS-1]) {
    currentPosition = 0;
  }
  targetPosition = currentPosition;
  
  Serial.print("Current Position loaded: ");
  Serial.println(currentPosition);
}

void loop() {

  if (espSerial.available() > 0) {
    
    String input = espSerial.readStringUntil('\n'); 
    input.trim(); // Remove whitespace

    Serial.print("[DEBUG] Raw Input from ESP32: ");
    Serial.println(input);

    if (input.length() > 0) {
      int floor = input.toInt();

      if (floor >= 0 && floor < NUM_FLOORS) {
        targetPosition = floorSteps[floor];
        
        Serial.print("[DEBUG] Valid Floor! Moving to step: ");
        Serial.println(targetPosition);
      } else {
        Serial.println("[DEBUG] ERROR: Invalid Floor Number");
      }
    }
  }

  if (currentPosition < targetPosition) {
    moveStep(true); 
  } 
  else if (currentPosition > targetPosition) {
    moveStep(false); 
  }

  static unsigned long lastSave = 0;
  if (millis() - lastSave > 5000 && currentPosition == targetPosition) {
    EEPROM.put(ORIGIN_ADDR, currentPosition);
    lastSave = millis();
  }
}

void moveStep(bool up) {
 
  digitalWrite(DIR_PIN, up ? LOW : HIGH);  
  delayMicroseconds(50); 

  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(50);  
  digitalWrite(STEP_PIN, LOW);
  
  delayMicroseconds(stepDelay);

  currentPosition += up ? 1 : -1;
}