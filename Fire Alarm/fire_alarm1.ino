#include "DHT.h"

#define DHTTYPE DHT11

// --- Pin Definitions ---
#define DHTPIN1 2
#define DHTPIN2 31
#define DHTPIN3 33
 
#define DHTPIN4 32

#define WEB_TRIGGER_PIN A0    
#define FEEDBACK_PIN 51        
#define BUZZERPIN 52
#define BUZZERPIN2 35                                                                                                                                                                                                         
#define LEDPIN1 10 
#define LEDPIN2 11
#define LEDPIN3 12
#define LEDPIN4 8
#define LEDPIN5 9
#define LEDPIN6 13
#define LEDPIN7 7
#define LEDPIN8 6
#define LEDPIN9 5
#define LEDPIN10 4
#define PUMPPIN 53             

DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);
DHT dht3(DHTPIN3, DHTTYPE);
DHT dht4(DHTPIN4, DHTTYPE);

// --- PUMP TIMER VARIABLES ---
unsigned long pumpStartTime = 0;  
bool pumpIsRunning = false;       
bool pumpHasFinished = false;      

// --- ALARM TIMER VARIABLES ---
unsigned long alarmStartTime = 0;
bool alarmTimerActive = false;    
const unsigned long ALARM_TIMEOUT = 45000; // 45 Seconds limit

void setup() {
  Serial.begin(9600);

  dht1.begin(); dht2.begin(); dht3.begin(); dht4.begin();

  pinMode(BUZZERPIN, OUTPUT);
  pinMode(BUZZERPIN2, OUTPUT);
  pinMode(LEDPIN1, OUTPUT);
  pinMode(LEDPIN2, OUTPUT);
  pinMode(LEDPIN3, OUTPUT);
  pinMode(LEDPIN4, OUTPUT);
  pinMode(LEDPIN5, OUTPUT);
  pinMode(LEDPIN6, OUTPUT);
  pinMode(LEDPIN7, OUTPUT);
  pinMode(LEDPIN8, OUTPUT);
  pinMode(LEDPIN9, OUTPUT);
  pinMode(LEDPIN10, OUTPUT);
  pinMode(PUMPPIN, OUTPUT);
  digitalWrite(PUMPPIN, LOW); 
  
  pinMode(WEB_TRIGGER_PIN, INPUT); 
  
  // UPDATED: Set Feedback to OUTPUT and default to LOW
  pinMode(FEEDBACK_PIN, OUTPUT); 
  digitalWrite(FEEDBACK_PIN, LOW);
}

void policeSiren() {
  static unsigned long lastBlinkTime = 0;
  static bool ledState = LOW;
  const int blinkInterval = 300;

  // --- Sweep Up ---
  for (int freq = 450; freq <= 1200; freq += 10) {
    tone(BUZZERPIN, freq);
    tone(BUZZERPIN2, freq);
    if (millis() - lastBlinkTime >= blinkInterval) {
      ledState = !ledState;
      digitalWrite(LEDPIN1, ledState); digitalWrite(LEDPIN2, ledState);
      digitalWrite(LEDPIN3, ledState); digitalWrite(LEDPIN4, ledState);
      digitalWrite(LEDPIN5, ledState); digitalWrite(LEDPIN6, ledState);
      digitalWrite(LEDPIN7, ledState); digitalWrite(LEDPIN8, ledState);
      digitalWrite(LEDPIN9, ledState); digitalWrite(LEDPIN10, ledState);
      lastBlinkTime = millis();
    }
    delay(2);
  }

  // --- Sweep Down ---
  for (int freq = 1200; freq >= 450; freq -= 10) {
    tone(BUZZERPIN, freq);
    tone(BUZZERPIN2, freq);
    if (millis() - lastBlinkTime >= blinkInterval) {
      ledState = !ledState;
      digitalWrite(LEDPIN1, ledState); digitalWrite(LEDPIN2, ledState);
      digitalWrite(LEDPIN3, ledState); digitalWrite(LEDPIN4, ledState);
      digitalWrite(LEDPIN5, ledState); digitalWrite(LEDPIN6, ledState);
      digitalWrite(LEDPIN7, ledState); digitalWrite(LEDPIN8, ledState);
      digitalWrite(LEDPIN9, ledState); digitalWrite(LEDPIN10, ledState);
      lastBlinkTime = millis();
    }
    delay(2);
  }
}

  
void loop() {
  // 1. Read Web Signal
  int webSignalValue = analogRead(WEB_TRIGGER_PIN);
  bool webAlarm = (webSignalValue > 400);

  // 2. Read Sensors
  float t1 = dht1.readTemperature();
  float t2 = dht2.readTemperature();
  float t3 = dht3.readTemperature();
  float t4 = dht4.readTemperature();

  if (isnan(t1)) t1 = 0;
  if (isnan(t2)) t2 = 0;
  if (isnan(t3)) t3 = 0;
  if (isnan(t4)) t4 = 0;

  bool sensorAlarm = (t1 > 20 || t2 > 20 || t3 > 20 || t4 > 20);

  // --- SMART PUMP LOGIC (30 Second Timer) ---
  if (sensorAlarm) {
    if (!pumpIsRunning && !pumpHasFinished) {
      digitalWrite(PUMPPIN, HIGH);   
      
      // UPDATED: Turn ON Feedback and LED when Pump Starts
      digitalWrite(FEEDBACK_PIN, HIGH); 
      digitalWrite(LEDPIN1, HIGH);
      digitalWrite(LEDPIN2, HIGH);
      digitalWrite(LEDPIN3, HIGH);
      digitalWrite(LEDPIN4, HIGH);
      digitalWrite(LEDPIN5, HIGH);
      digitalWrite(LEDPIN6, HIGH);
      digitalWrite(LEDPIN7, HIGH);
      digitalWrite(LEDPIN8, HIGH);
      digitalWrite(LEDPIN9, HIGH);
      digitalWrite(LEDPIN10, HIGH);
 
      
      pumpStartTime = millis();      
      pumpIsRunning = true;
      Serial.println("💦 Pump STARTED (30s Timer)");
    }

    if (pumpIsRunning) {
      if (millis() - pumpStartTime >= 30000) {
        digitalWrite(PUMPPIN, LOW);  
        
        // UPDATED: Turn OFF Feedback when Pump Times Out
        digitalWrite(FEEDBACK_PIN, LOW); 
        
        pumpIsRunning = false;
        pumpHasFinished = true;       
        Serial.println("🛑 Pump STOPPED (Timeout)");
      }
    }
  } else {
    digitalWrite(PUMPPIN, LOW);
    
    // UPDATED: Ensure Feedback is LOW if there is no alarm
    digitalWrite(FEEDBACK_PIN, LOW); 
    
    pumpIsRunning = false;
    pumpHasFinished = false; 
  }

  // --- ALARM TIMEOUT LOGIC ---
  bool enableSiren = false;

  if (sensorAlarm) {
    if (!alarmTimerActive) {
      alarmStartTime = millis();
      alarmTimerActive = true;
    }

    if (millis() - alarmStartTime < ALARM_TIMEOUT) {
      enableSiren = true; 
    } else {
      enableSiren = false; 
      Serial.println("🔕 Sensor Alarm Muted (45s Timeout)");
    }
  } else {
    alarmTimerActive = false;
  }

  // Web Alarm Override
  if (webAlarm) {
    enableSiren = true; 
    Serial.println("🚨 WEB OVERRIDE ALARM");
  }

  // --- MASTER OUTPUT ---
  if (enableSiren) {
    if(sensorAlarm && alarmTimerActive && (millis() - alarmStartTime < ALARM_TIMEOUT)) {
       Serial.println("🚨 SENSOR ALARM ACTIVE!");
    }
    
    // Removed pinMode changing here. Just play sound.
    // The LED will pulse because of the analogWrite inside policeSiren
    policeSiren(); 
  } else {
    // Safe State / Silenced State
    
    // UPDATED: Ensure Feedback stays LOW in safe state
    digitalWrite(FEEDBACK_PIN, LOW); 
    
    noTone(BUZZERPIN);
    noTone(BUZZERPIN2);
    analogWrite(LEDPIN1, 0);
    analogWrite(LEDPIN2, 0);
    analogWrite(LEDPIN3, 0);
    analogWrite(LEDPIN4, 0);
    analogWrite(LEDPIN5, 0);
    analogWrite(LEDPIN6, 0);
    analogWrite(LEDPIN7, 0);
    analogWrite(LEDPIN8, 0);
    analogWrite(LEDPIN9, 0);
    analogWrite(LEDPIN10, 0);

    delay(200); 
  }
}