#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Pin definitions
#define SS_PIN 10
#define RST_PIN 9
#define LED_GREEN_PIN 8
#define LED_RED_PIN 6
#define BUZZER_PIN 7

// ESP32 communication pins
#define ESP32_RX_PIN 2
#define ESP32_TX_PIN 3

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial esp32Serial(ESP32_RX_PIN, ESP32_TX_PIN);

// Authorized users database
struct AuthorizedUser {
  byte uid[4];
  String name;
  String id;
  String department;
  bool isActive;
};

AuthorizedUser authorizedUsers[] = {
  {{0x9B, 0x3D, 0x42, 0x05}, "Taz", "202314100", "CSE", true},
  {{0x6D, 0x7E, 0x6A, 0x05}, "Jamil", "202314102", "CSE", true},
  {{0x77, 0xB7, 0x47, 0x05}, "Tamim", "202314083", "CSE", true}
};

const int NUM_USERS = sizeof(authorizedUsers) / sizeof(authorizedUsers[0]);

// System state
bool waitingForFaceVerification = false;
String currentUser = "";
String currentUserID = "";
unsigned long faceRequestTime = 0;
const unsigned long FACE_TIMEOUT = 15000; // 15 seconds timeout

void setup() {
  Serial.begin(9600);
  esp32Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Initialize pins
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // Startup sequence
  showStartupMessage();
  testHardware();
  showReadyMessage();
  
  Serial.println("=== Arduino Uno RFID System Ready ===");
  Serial.println("Waiting for RFID cards...");
}

void loop() {
  // Handle face verification timeout
  if (waitingForFaceVerification && (millis() - faceRequestTime > FACE_TIMEOUT)) {
    handleFaceTimeout();
  }
  
  // Check for ESP32 response
  if (esp32Serial.available()) {
    handleESP32Response();
  }
  
  // Check for new RFID card
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    handleRFIDScan();
  }
  
  delay(100);
}

void handleRFIDScan() {
  if (waitingForFaceVerification) {
    // Already processing another user
    showMessage("Please wait...", "Processing user");
    delay(1000);
    return;
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning card...");
  
  int userIndex = checkAuthorized(mfrc522.uid.uidByte, mfrc522.uid.size);
  
  if (userIndex >= 0) {
    // RFID authorized
    AuthorizedUser user = authorizedUsers[userIndex];
    
    Serial.println("RFID_AUTHORIZED");
    Serial.println("User: " + user.name);
    Serial.println("ID: " + user.id);
    
    // Show user info
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hello " + user.name);
    lcd.setCursor(0, 1);
    lcd.print("ID: " + user.id);
    
    // Green LED and success beep
    digitalWrite(LED_GREEN_PIN, HIGH);
    successBeep();
    delay(2000);
    digitalWrite(LED_GREEN_PIN, LOW);
    
    // Request face verification from ESP32
    requestFaceVerification(user.name, user.id);
    
  } else {
    // RFID unauthorized
    Serial.println("RFID_DENIED");
    Serial.println("Unknown card: " + getUIDString(mfrc522.uid.uidByte, mfrc522.uid.size));
    
    showMessage("ACCESS DENIED", "Unknown card");
    
    // Red LED and error beep
    digitalWrite(LED_RED_PIN, HIGH);
    errorBeep();
    delay(2000);
    digitalWrite(LED_RED_PIN, LOW);
    
    showReadyMessage();
  }
  
  mfrc522.PICC_HaltA();
}

void requestFaceVerification(String name, String id) {
  currentUser = name;
  currentUserID = id;
  waitingForFaceVerification = true;
  faceRequestTime = millis();
  
  // Send request to ESP32
  esp32Serial.println("FACE_REQUEST");
  esp32Serial.println("NAME:" + name);
  esp32Serial.println("ID:" + id);
  esp32Serial.println("END_REQUEST");
  
  // Show waiting message
  showMessage("Face verification", "Look at camera");
  
  Serial.println("Face verification requested for: " + name);
}

void handleESP32Response() {
  String response = "";
  
  // Read complete response
  while (esp32Serial.available()) {
    char c = esp32Serial.read();
    if (c == '\n') {
      processESP32Response(response);
      response = "";
    } else if (c != '\r') {
      response += c;
    }
  }
}

void processESP32Response(String response) {
  Serial.println("ESP32 Response: " + response);
  
  if (response.startsWith("FACE_VERIFIED:")) {
    String verifiedName = response.substring(14);
    handleFaceSuccess(verifiedName);
    
  } else if (response == "FACE_NOT_FOUND") {
    handleFaceFailure("Face not detected");
    
  } else if (response == "FACE_UNKNOWN") {
    handleFaceFailure("Unknown face");
    
  } else if (response.startsWith("FACE_CONFIDENCE:")) {
    float confidence = response.substring(16).toFloat();
    if (confidence < 70) {
      handleFaceFailure("Low confidence");
    }
  } else if (response == "SHEETS_SUCCESS") {
    handleSheetsSuccess();
  } else if (response == "SHEETS_FAILED") {
    handleSheetsFailure();
  }
}

void handleFaceSuccess(String verifiedName) {
  if (verifiedName == currentUser) {
    // Face matches RFID user
    Serial.println("ATTENDANCE_SUCCESS: " + currentUser);
    
    showMessage("VERIFIED!", currentUser + " present");
    
    // Success indicators
    digitalWrite(LED_GREEN_PIN, HIGH);
    successBeep();
    delay(3000);
    digitalWrite(LED_GREEN_PIN, LOW);
    
  } else {
    // Face doesn't match RFID user
    handleFaceFailure("Face mismatch");
  }
  
  resetSystem();
}

void handleFaceFailure(String reason) {
  Serial.println("FACE_FAILED: " + reason);
  
  showMessage("VERIFICATION", "FAILED!");
  lcd.setCursor(0, 1);
  lcd.print(reason);
  
  // Error indicators
  digitalWrite(LED_RED_PIN, HIGH);
  errorBeep();
  delay(3000);
  digitalWrite(LED_RED_PIN, LOW);
  
  resetSystem();
}

void handleFaceTimeout() {
  Serial.println("FACE_TIMEOUT for user: " + currentUser);
  
  showMessage("TIMEOUT!", "Try again");
  
  // Warning indicators
  digitalWrite(LED_RED_PIN, HIGH);
  warningBeep();
  delay(2000);
  digitalWrite(LED_RED_PIN, LOW);
  
  resetSystem();
}

void handleSheetsSuccess() {
  showMessage("Data saved to", "Google Sheets!");
  delay(2000);
  showReadyMessage();
}

void handleSheetsFailure() {
  showMessage("Warning:", "Data not saved");
  delay(2000);
  showReadyMessage();
}

void resetSystem() {
  waitingForFaceVerification = false;
  currentUser = "";
  currentUserID = "";
  faceRequestTime = 0;
  
  delay(1000);
  showReadyMessage();
}

int checkAuthorized(byte *uid, byte uidSize) {
  if (uidSize != 4) return -1;
  
  for (int i = 0; i < NUM_USERS; i++) {
    if (!authorizedUsers[i].isActive) continue;
    
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (uid[j] != authorizedUsers[i].uid[j]) {
        match = false;
        break;
      }
    }
    if (match) return i;
  }
  return -1;
}

void showStartupMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Attendance Sys");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
}

void testHardware() {
  // Test LEDs
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Testing LEDs...");
  
  digitalWrite(LED_GREEN_PIN, HIGH);
  delay(300);
  digitalWrite(LED_GREEN_PIN, LOW);
  
  digitalWrite(LED_RED_PIN, HIGH);
  delay(300);
  digitalWrite(LED_RED_PIN, LOW);
  
  // Test buzzer
  lcd.setCursor(0, 1);
  lcd.print("Testing buzzer..");
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  
  delay(1000);
}

void showReadyMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place RFID card");
  lcd.setCursor(0, 1);
  lcd.print("on the reader");
}

void showMessage(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void successBeep() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
  }
}

void errorBeep() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

void warningBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);
}

String getUIDString(byte *uid, byte uidSize) {
  String uidStr = "";
  for (byte i = 0; i < uidSize; i++) {
    if (uid[i] < 0x10) uidStr += "0";
    uidStr += String(uid[i], HEX);
    if (i < uidSize - 1) uidStr += ":";
  }
  uidStr.toUpperCase();
  return uidStr;
}