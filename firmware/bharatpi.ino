#include <esp_now.h>
#include <WiFi.h>
#include <math.h>

// ==========================================
// ⚙️ HARDWARE CONFIG
// ==========================================
// This Pin sends the signal to the Arduino
#define ARDUINO_TRIGGER_PIN 21 

// AI MEMORY SIZE
#define WINDOW_SIZE 50

// ==========================================
// 🧠 TINYML ENGINE (Z-SCORE)
// ==========================================
class EdgeAI {
  private:
    float buffer[WINDOW_SIZE];
    int head = 0;
    int count = 0;
  public:
    void push(float val) {
      buffer[head] = val;
      head = (head + 1) % WINDOW_SIZE;
      if (count < WINDOW_SIZE) count++;
    }
    float getZScore(float val) {
      if (count < 10) return 0.0; // Training phase
      
      float sum = 0;
      for(int i=0; i<count; i++) sum += buffer[i];
      float mean = sum / count;
      
      float sumSq = 0;
      for(int i=0; i<count; i++) sumSq += pow(buffer[i] - mean, 2);
      float std = sqrt(sumSq / count);
      
      if (std == 0) return 0.0;
      return abs((val - mean) / std);
    }
};

EdgeAI ai;

// ==========================================
// 📦 DATA PACKET
// ==========================================
typedef struct struct_message {
  int id;
  float value;
  bool fault;
} struct_message;

struct_message incoming;

// ==========================================
// 📡 RECEIVE CALLBACK
// ==========================================
// Note: Using (const uint8_t *) for compatibility
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incoming, incomingData, sizeof(incoming));
  
  float val = incoming.value;
  
  // 1. RUN AI INFERENCE
  ai.push(val);
  float risk = ai.getZScore(val);
  
  // 2. PHYSICAL ACTUATION (The Handshake)
  if (risk > 4.0) {
    // CRITICAL: Pull Pin 21 HIGH (3.3V)
    // This tells Arduino to fire the SMS
    digitalWrite(ARDUINO_TRIGGER_PIN, HIGH); 
  } else {
    // SAFE: Keep Pin 21 LOW (0V)
    digitalWrite(ARDUINO_TRIGGER_PIN, LOW);
  }

  // 3. DASHBOARD TELEMETRY (Serial to Laptop)
  // Format: "ID:1,Telemetry:45.5,RiskScore:0.1"
  Serial.print("ID:"); Serial.print(incoming.id);
  Serial.print(",Telemetry:"); Serial.print(val);
  Serial.print(",RiskScore:"); Serial.println(risk);
}

void setup() {
  Serial.begin(115200);
  
  // Init Trigger Pin
  pinMode(ARDUINO_TRIGGER_PIN, OUTPUT);
  
  // --- STARTUP TEST (Handshake Check) ---
  // We blink Pin 21 High for 500ms. 
  // IF WIRED CORRECTLY: Arduino Green LED should flick OFF then ON.
  Serial.println("Testing Link to Watchdog...");
  digitalWrite(ARDUINO_TRIGGER_PIN, HIGH);
  delay(500); 
  digitalWrite(ARDUINO_TRIGGER_PIN, LOW);
  Serial.println("Link Test Complete.");
  // ---------------------------------------

  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register Callback (Cast to fix compiler errors on new versions)
  esp_now_register_recv_cb((esp_now_recv_cb_t)OnDataRecv);
  
  Serial.println("QUEEN ONLINE. READY FOR CHAOS.");
}

void loop() { delay(1000); }