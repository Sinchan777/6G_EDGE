#include <esp_now.h>
#include <WiFi.h>

// ============================================================
// ⚙️ CONFIGURATION SECTION
// ============================================================

// 1. PASTE THE MAC ADDRESS OF YOUR BHARATPI (QUEEN) HERE
// Format: {0x24, 0x6F, 0x28, 0xA1, 0xB2, 0xC3} 
// (Replace FF with your actual numbers)
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// 2. NODE IDENTITY (CRITICAL STEP!)
// Set to '1' for the First Board (Thermal Sensor)
// Set to '2' for the Second Board (RF Signal Sensor)
int node_id = 2; 

// ============================================================

#define BUTTON_PIN 0  // Built-in BOOT Button on ESP32

// Data Packet Structure (Must match Receiver exactly)
typedef struct struct_message {
  int id;
  float value;
  bool fault;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// Physics Simulation State
float time_step = 0;
bool fault_mode = false;

// Callback function (Required for ESP-NOW to work)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Optional: You can blink an LED here if you want visual feedback on send
}

void setup() {
  Serial.begin(115200);
  
  // 1. Hardware Init
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  WiFi.mode(WIFI_STA); // Station Mode is required for ESP-NOW
  
  // 2. Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  
  // 3. Register Peer (The Queen/BharatPi)
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.print("Digital Twin Online. ID: ");
  Serial.println(node_id);
}

void loop() {
  // A. CHECK FOR CHAOS TRIGGER (Human Input)
  // If BOOT button is held down, we act like a broken/hacked tower
  if (digitalRead(BUTTON_PIN) == LOW) {
    fault_mode = true;
  } else {
    fault_mode = false;
  }

  // B. PHYSICS ENGINE (Simulate 6G Tower Data)
  // Generate a Sine Wave (Day/Night traffic cycle) + Random Noise
  float noise = (random(-10, 10) / 10.0);
  float wave = 5.0 * sin(time_step * 0.1);

  if (fault_mode) {
    // ATTACK MODE: Generate Critical Spike
    // Simulates Thermal Runaway (>100C) or Signal Jamming
    myData.value = 120.0 + (random(0, 500)/10.0);
  } else {
    // NORMAL MODE: Generate Baseline Data
    // Simulates normal operating temp (~45C)
    myData.value = 45.0 + wave + noise;
  }

  // C. PREPARE PACKET
  myData.id = node_id;
  myData.fault = fault_mode;

  // D. SEND VIA MESH (ESP-NOW)
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  
  // Debug Output
  if (result == ESP_OK) {
    Serial.print("Node:"); Serial.print(node_id);
    Serial.print(" | Sending Val:"); Serial.print(myData.value);
    Serial.print(" | Fault:"); Serial.println(fault_mode ? "ACTIVE" : "NONE");
  }
  else {
    Serial.println("Error sending data (Check MAC Address?)");
  }
  
  time_step += 1.0;
  delay(100); // 10Hz Refresh Rate (Fast Edge Updates)
}