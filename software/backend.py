import serial
import time
import re
import threading
from flask import Flask, jsonify
from flask_cors import CORS

# ==========================================
# ⚙️ CONFIGURATION
# ==========================================
# CHANGE THIS TO MATCH YOUR BHARATPI PORT
# Windows Example: 'COM3'
# Linux/Mac Example: '/dev/ttyUSB0'
SERIAL_PORT = '/dev/ttyUSB0'  
BAUD_RATE = 115200
# ==========================================

app = Flask(__name__)
CORS(app) # Enable Cross-Origin Resource Sharing for the HTML frontend

# Global State with Timestamp to track freshness
current_state = {
    "id": 0,
    "temp": 0.0,
    "risk": 0.0,
    "status": "WAITING",
    "last_update": 0  # <--- NEW: Tracks when the last valid packet arrived
}

def read_serial_loop():
    """
    Background thread: Reads USB Serial data.
    Updates 'current_state' ONLY when valid data arrives.
    """
    global current_state
    
    while True:
        try:
            print(f"🔌 Connecting to {SERIAL_PORT}...")
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            print("✅ Connected! Listening for Telemetry...")
            
            while True:
                if ser.in_waiting > 0:
                    # Read line from BharatPi
                    # Expected Format: "ID:1,Telemetry:45.5,RiskScore:0.1"
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    
                    # Robust Regex Parsing
                    id_match = re.search(r'ID:(\d+)', line)
                    temp_match = re.search(r'Telemetry:([\d\.]+)', line)
                    risk_match = re.search(r'RiskScore:([\d\.]+)', line)
                    
                    if temp_match and risk_match:
                        # Extract values
                        t = float(temp_match.group(1))
                        r = float(risk_match.group(1))
                        node_id = int(id_match.group(1)) if id_match else 0
                        
                        # Determine Status Logic locally as a fallback
                        status = "NORMAL"
                        if r > 4.0: status = "CRITICAL"
                        
                        # Update the Global State with current time
                        current_state = {
                            "id": node_id,
                            "temp": t,
                            "risk": r,
                            "status": status,
                            "last_update": time.time() # <--- TIMESTAMP UPDATE
                        }
                        print(f"📥 Received: {current_state}")
                        
        except Exception as e:
            print(f"⚠️ Serial Error: {e}")
            print("🔄 Retrying in 2 seconds...")
            time.sleep(2)

# Start the Serial Reader Thread
threading.Thread(target=read_serial_loop, daemon=True).start()

@app.route('/data')
def get_data():
    """
    API Endpoint: Returns JSON data to the dashboard.
    Includes a 'Freshness Check' to detect disconnections.
    """
    # 1. Check if data is stale (older than 3 seconds)
    if time.time() - current_state['last_update'] > 3:
        return jsonify({
            "id": 0, 
            "temp": 0, 
            "risk": 0, 
            "status": "OFFLINE/DISCONNECTED" # <--- FORCE OFFLINE STATUS
        })
    
    # 2. If fresh, return actual data
    return jsonify(current_state)

if __name__ == '__main__':
    print("🚀 Bridge Server Running on http://localhost:5000")
    app.run(host='0.0.0.0', port=5000)