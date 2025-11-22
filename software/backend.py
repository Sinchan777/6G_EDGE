import serial
import time
import re
import threading
from flask import Flask, jsonify
from flask_cors import CORS

# --- CONFIG ---
SERIAL_PORT = 'COM3' # <--- CHECK THIS AGAIN
BAUD_RATE = 115200
# --------------

app = Flask(__name__)
CORS(app)

current_state = {"id": 0, "temp": 0.0, "risk": 0.0, "status": "WAITING", "last_update": 0}

def read_serial_loop():
    global current_state
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            print(f"✅ Connected to {SERIAL_PORT}")
            
            while True:
                if ser.in_waiting:
                    # READ RAW LINE
                    raw_line = ser.readline().decode('utf-8', errors='ignore').strip()
                    
                    # PRINT IT (DEBUGGING)
                    if "RiskScore" in raw_line:
                        print(f"RAW DATA: {raw_line}") # <--- WATCH THIS IN TERMINAL
                    
                    # PARSE IT
                    id_match = re.search(r'ID:(\d+)', raw_line)
                    temp_match = re.search(r'Telemetry:([\d\.]+)', raw_line)
                    risk_match = re.search(r'RiskScore:([\d\.]+)', raw_line)
                    
                    if temp_match and risk_match:
                        t = float(temp_match.group(1))
                        r = float(risk_match.group(1))
                        node_id = int(id_match.group(1)) if id_match else 1
                        
                        status = "NORMAL"
                        if r > 4.0: status = "CRITICAL"
                        
                        current_state = {
                            "id": node_id, "temp": t, "risk": r, "status": status, "last_update": time.time()
                        }
                        
                        # PRINT SUCCESS
                        if status == "CRITICAL":
                            print(">>> CRITICAL UPDATE DETECTED! <<<")

        except Exception as e:
            print(f"Serial Error: {e}")
            time.sleep(2)

threading.Thread(target=read_serial_loop, daemon=True).start()

@app.route('/data')
def get_data():
    # If no data for 3 seconds, go OFFLINE
    if time.time() - current_state['last_update'] > 3:
        return jsonify({"id": 0, "temp": 0, "risk": 0, "status": "OFFLINE"})
    return jsonify(current_state)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
```

**Run this.**
1.  Press the button on the Twin.
2.  Look at the Python Terminal.
3.  **If you see:** `RAW DATA: ID:1,Telemetry:150.0,RiskScore:12.5`
    * But the Dashboard is still green...
    * **Then the issue is the Browser Cache.** Refresh the page `Ctrl+F5`.

---

### **Step 3: The "Nuclear" Fix (Sync the Code)**
If Step 1 showed "Scenario A" (Queen not hearing Twin), here is the most common mistake:

**The ESP-NOW Channel Mismatch.**
If your home WiFi is on Channel 1, and ESP-NOW tries Channel 1, but your Laptop forces the BharatPi to Channel 6... the packets get lost.

**Fix:** Update the **Twin Code** to cycle through channels until it finds the Queen. Or, simpler for the hackathon:

**Update the Twin `setup()` code to this:**

```cpp
// Inside esp32_twin_sender.ino

void setup() {
  // ... (Init lines) ...
  
  WiFi.mode(WIFI_STA);
  
  // FORCE CHANNEL 1 (Add this line)
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) { return; }
  
  // ... (Rest of code) ...
}
```

**And Update the Queen `setup()` code to this:**

```cpp
// Inside bharatpi_queen.ino

void setup() {
  // ... (Init lines) ...

  WiFi.mode(WIFI_STA);

  // FORCE CHANNEL 1 (Add this line)
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) { return; }

  // ... (Rest of code) ...
}
