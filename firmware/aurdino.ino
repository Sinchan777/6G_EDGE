#include <SoftwareSerial.h>

// --- CONFIGURATION ---
// SIM800L Connection (Software Serial)
// Arduino D10 -> SIM800L TX
// Arduino D11 -> SIM800L RX
SoftwareSerial gsm(10, 11); 

// Trigger from BharatPi
#define TRIGGER_PIN 4

// Status LEDs
#define GREEN_LED 8
#define RED_LED 9

// SMS Settings
const char* PHONE_NUMBER = "+919242974455"; // ⚠️ REPLACE WITH YOUR NUMBER
unsigned long last_sms_time = 0;
const long SMS_COOLDOWN = 120000; // 2 Minutes Cooldown

void setup() {
  // Init Serial for Debugging (USB)
  Serial.begin(9600);
  
  // Init GSM Serial
  gsm.begin(9600); 
  
  // Init Pins
  pinMode(TRIGGER_PIN, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  
  Serial.println("INITIALIZING WATCHDOG + GSM...");
  
  // Wake up GSM & Set Text Mode
  delay(1000);
  gsm.println("AT");
  delay(1000);
  gsm.println("AT+CMGF=1"); // Set SMS Logic to Text Mode
  delay(1000);
  
  // Startup Flash (Visual Check)
  digitalWrite(GREEN_LED, HIGH); delay(200); digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH); delay(200); digitalWrite(RED_LED, LOW);
  Serial.println("SYSTEM READY.");
}

void send_emergency_sms() {
  // Check Cooldown
  if (millis() - last_sms_time < SMS_COOLDOWN) {
    Serial.println("SMS Skipped (Cooldown Active)");
    return;
  }

  Serial.println(">>> SENDING CRITICAL SMS <<<");
  
  // AT Command Sequence
  gsm.print("AT+CMGS=\"");
  gsm.print(PHONE_NUMBER);
  gsm.println("\"");
  delay(1000);
  
  gsm.print("CRITICAL ALERT: 6G Base Station Anomaly Detected! AI Risk Score > 4.0. Immediate Action Required.");
  delay(100);
  
  gsm.write(26); // CTRL+Z ASCII code to send
  delay(5000); // Wait for network
  
  Serial.println("SMS SENT.");
  last_sms_time = millis();
}

void loop() {
  // Read Trigger from BharatPi
  int state = digitalRead(TRIGGER_PIN);
  
  if (state == HIGH) {
    // ========================
    // CRITICAL FAULT STATE
    // ========================
    digitalWrite(GREEN_LED, LOW);
    
    // 1. Strobe Red LED
    digitalWrite(RED_LED, HIGH); delay(50);
    digitalWrite(RED_LED, LOW); delay(50);
    
    // 2. Trigger GSM
    send_emergency_sms();
    
  } else {
    // ========================
    // NORMAL STATE
    // ========================
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    
    // Reset cooldown if system normalizes? 
    // No, keep cooldown to avoid fluttering triggers.
  }
  
  // Debug GSM Buffer (Optional: Print GSM responses to Serial Monitor)
  if (gsm.available()) {
    Serial.write(gsm.read());
  }
}