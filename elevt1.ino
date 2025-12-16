#include <EEPROM.h>
#include <SoftwareSerial.h> // Include the library

// ========== PINS ==========
#define STEP_PIN 5
#define DIR_PIN  4
#define EN_PIN   2

// ========== SOFTWARE SERIAL PINS ==========
// Connect ESP32 TX to Arduino Pin 10
const int RxPin = 10; 
const int TxPin = 11; // Not used, but required by library
SoftwareSerial espSerial(RxPin, TxPin);

// ========== MOTOR SETTINGS ==========
int stepDelay = 2000;           // microseconds per step
long currentPosition = 0;       // current step
long targetPosition = 0;        // target step

// ========== FLOOR MAP ==========
#define NUM_FLOORS 10
long floorSteps[NUM_FLOORS] = {930, 1800, 2500, 3400, 4150, 4950, 5750, 6550, 7350, 8150};

// ========== EEPROM ==========
#define ORIGIN_ADDR 0

void setup() {
  // 1. Start USB Serial for DEBUGGING (Computer)
  Serial.begin(9600);
  Serial.println("--- SYSTEM START ---");
  Serial.println("Waiting for data from ESP32 on Pin 10...");

  // 2. Start Software Serial for ESP32
  espSerial.begin(9600);
  espSerial.setTimeout(50); // Don't wait too long for data

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW); // enable driver

  // Load last position
  EEPROM.get(ORIGIN_ADDR, currentPosition);
  
  // Safety check
  if (currentPosition < 0 || currentPosition > floorSteps[NUM_FLOORS-1]) {
    currentPosition = 0;
  }
  targetPosition = currentPosition;
  
  Serial.print("Current Position loaded: ");
  Serial.println(currentPosition);
}

void loop() {
  // ========== READ FROM ESP32 (Software Serial) ==========
  if (espSerial.available() > 0) {
    
    // Read the string from ESP32
    String input = espSerial.readStringUntil('\n'); 
    input.trim(); // Remove whitespace

    // --- DEBUGGING: Print to Computer ---
    Serial.print("[DEBUG] Raw Input from ESP32: ");
    Serial.println(input);
    // ------------------------------------

    if (input.length() > 0) {
      int floor = input.toInt(); // Convert to integer

      // Validate floor number
      if (floor >= 0 && floor < NUM_FLOORS) {
        targetPosition = floorSteps[floor];
        
        Serial.print("[DEBUG] Valid Floor! Moving to step: ");
        Serial.println(targetPosition);
      } else {
        Serial.println("[DEBUG] ERROR: Invalid Floor Number");
      }
    }
  }

  // ========== MOVE MOTOR ==========
  if (currentPosition < targetPosition) {
    moveStep(true);   // moving "up"
  } 
  else if (currentPosition > targetPosition) {
    moveStep(false);  // moving "down"
  }

  // ========== SAVE POSITION ==========
  static unsigned long lastSave = 0;
  if (millis() - lastSave > 5000 && currentPosition == targetPosition) {
    EEPROM.put(ORIGIN_ADDR, currentPosition);
    lastSave = millis();
    // Serial.println("[DEBUG] Position Saved to EEPROM"); // Optional debug
  }
}

// ========== STEP FUNCTION ==========
void moveStep(bool up) {
  // Set Direction
  digitalWrite(DIR_PIN, up ? LOW : HIGH);  
  delayMicroseconds(50); 

  // Step Pulse
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(50);  
  digitalWrite(STEP_PIN, LOW);
  
  // Speed control
  delayMicroseconds(stepDelay);

  // Update position tracker
  currentPosition += up ? 1 : -1;
}