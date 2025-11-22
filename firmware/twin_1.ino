#include <esp_now.h>
#include <WiFi.h>

// ============================================================
// ⚠️ STEP 1: PASTE BHARATPI MAC ADDRESS HERE
// Format: {0x24, 0x6F, 0x28, 0xA1, 0xB2, 0xC3}
//08:D1:F9:19:30:88
uint8_t broadcastAddress[] = {0x08, 0xD1, 0xF9, 0x19, 0x30, 0x88};

// ============================================================
// ⚠️ STEP 2: CHANGE THIS ID FOR EACH BOARD (1 or 2)
int node_id = 1; 
// ============================================================

#define BUTTON_PIN 0  // Built-in BOOT Button

// DATA PACKET STRUCTURE
typedef struct struct_message {
  int id;
  float value;
  bool fault;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// STATE VARIABLES
float time_step = 0;
unsigned long fault_timer = 0; // Timer to make the button "Sticky"

// Callback (Empty function needed for protocol)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {}

void setup() {
  Serial.begin(115200);
  
  // Init Hardware
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  WiFi.mode(WIFI_STA);
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // ✅ COMPATIBILITY FIX: Force the callback type for new ESP32 boards
  esp_now_register_send_cb((esp_now_send_cb_t)OnDataSent);
  
  // Register Peer (The Queen)
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("Twin Sensor Online.");
}

void loop() {
  // 1. STICKY BUTTON LOGIC
  // If button is pressed, trigger chaos for 2000ms (2 seconds)
  // This ensures the AI has enough time to catch the spike
  if (digitalRead(BUTTON_PIN) == LOW) {
    fault_timer = millis() + 2000; 
    Serial.println(">>> BUTTON PRESSED! INJECTING FAULT... <<<");
  }

  bool is_chaos_active = (millis() < fault_timer);

  // 2. PHYSICS SIMULATION
  // Reduced noise (0.5) to prevent accidental triggers during normal mode
  float noise = (random(-5, 5) / 10.0); 
  float wave = 5.0 * sin(time_step * 0.1);

  if (is_chaos_active) {
    // ATTACK MODE: Hardcoded high value to guarantee AI trigger
    myData.value = 150.0; 
  } else {
    // NORMAL MODE: Baseline + Wave + Light Noise
    myData.value = 45.0 + wave + noise;
  }

  myData.id = node_id;
  myData.fault = is_chaos_active;

  // 3. SEND DATA
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  
  // Debug Print
  Serial.print("ID:"); Serial.print(node_id);
  Serial.print(" | Val:"); Serial.print(myData.value);
  Serial.print(" | Mode:"); Serial.println(is_chaos_active ? "CHAOS" : "NORMAL");
  
  time_step += 1.0;
  delay(100); // 10Hz Refresh Rate
}