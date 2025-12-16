#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

// -------------------- RFID Readers --------------------
#define SS1 10
#define RST1 8
MFRC522 rfid1(SS1, RST1);

#define SS2 9
#define RST2 7
MFRC522 rfid2(SS2, RST2);

// -------------------- Servo --------------------
Servo myServo;
const int servoClosed = 0;
const int servoOpen = 90;
const unsigned long servoHoldTime = 3000;

// -------------------- Known UIDs --------------------
byte uid1[] = {0xD5, 0xCF, 0xE1, 0x00};
byte uid2[] = {0x82, 0x4D, 0xE1, 0x00};

// -------------------- Servo State --------------------
bool servoActive = false;
unsigned long servoStartTime = 0;

// *** ADDED ***: Define the new trigger pin from the ESP32
#define TRIGGER_PIN 4

// -------------------- Helper Functions --------------------
bool checkUID(byte *uid, byte *knownUID, byte length) {
  for (byte i = 0; i < length; i++) {
    if (uid[i] != knownUID[i]) return false;
  }
  return true;
}

void printUID(byte *uid, byte length) {
  Serial.print("UID (HEX): ");
  for (byte i = 0; i < length; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void openServo() {
  for (int i = 0; i <= 90; i++) {
    myServo.write(i);
    delay(5);
  }
  servoStartTime = millis();
  servoActive = true;
  Serial.println("Servo opened!");
}

void closeServo() {
  for (int i = 90; i >= 0; i--) {
    myServo.write(i);
    delay(5);
  }
  servoActive = false;
  Serial.println("Servo closed!");
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  SPI.begin();

  // *** ADDED ***: Set the trigger pin as an input with a pullup resistor
  // This means the pin is HIGH by default, and we wait for the ESP32
  // to pull it LOW.
  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  pinMode(SS1, OUTPUT);
  pinMode(SS2, OUTPUT);
  digitalWrite(SS1, HIGH);
  digitalWrite(SS2, HIGH);

  rfid1.PCD_Init();
  rfid2.PCD_Init();

  myServo.attach(6);
  myServo.write(servoClosed);

  Serial.println("RFID ready. Waiting for trigger on Pin 4...");
}

// -------------------- Main Loop --------------------
void loop() {

  // ---- 1. Check for Digital Trigger from ESP32 ----
  // *** CHANGED ***: This is the new, simple command check
  if (digitalRead(TRIGGER_PIN) == LOW) {
    Serial.println("Received Trigger from ESP32 on Pin 4!");
    if (!servoActive) {
      openServo();
    }
  }

  // ---- 2. Check Reader 1 ----
  digitalWrite(SS2, HIGH);
  digitalWrite(SS1, LOW);
  if (rfid1.PICC_IsNewCardPresent() && rfid1.PICC_ReadCardSerial()) {
    Serial.println("Reader 1 detected a tag!");
    printUID(rfid1.uid.uidByte, rfid1.uid.size);
    if ((checkUID(rfid1.uid.uidByte, uid1, 4) || checkUID(rfid1.uid.uidByte, uid2, 4)) && !servoActive) {
      openServo();
    }
    rfid1.PICC_HaltA();
  }
  digitalWrite(SS1, HIGH);

  // ---- 3. Check Reader 2 ----
  digitalWrite(SS1, HIGH);
  digitalWrite(SS2, LOW);
  if (rfid2.PICC_IsNewCardPresent() && rfid2.PICC_ReadCardSerial()) {
    Serial.println("Reader 2 detected a tag!");
    printUID(rfid2.uid.uidByte, rfid2.uid.size);
    if ((checkUID(rfid2.uid.uidByte, uid1, 4) || checkUID(rfid2.uid.uidByte, uid2, 4)) && !servoActive) {
      openServo();
    }
    rfid2.PICC_HaltA();
  }
  digitalWrite(SS2, HIGH);

  // ---- 4. Handle Servo Closing Timer ----
  if (servoActive && millis() - servoStartTime >= servoHoldTime) {
    closeServo();
  }
}