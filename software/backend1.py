import serial
import time
import re
import threading
from flask import Flask, jsonify
from flask_cors import CORS

# --- CONFIG ---
SERIAL_PORT = 'COM15' # CHECK THIS!
BAUD_RATE = 115200
# --------------

app = Flask(__name__)
# Allow ALL origins to prevent browser blocking
CORS(app, resources={r"/*": {"origins": "*"}})

# Global State
current_state = {
    "id": 0, "temp": 0.0, "risk": 0.0, 
    "status": "WAITING", "last_update": time.time()
}

def read_serial_loop():
    global current_state
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
            print(f"✅ Connected to {SERIAL_PORT}")
            ser.reset_input_buffer()
            
            while True:
                if ser.in_waiting:
                    try:
                        line = ser.readline().decode('utf-8', errors='ignore').strip()
                        
                        # Regex Parse
                        id_match = re.search(r'ID:(\d+)', line)
                        temp_match = re.search(r'Telemetry:([\d\.]+)', line)
                        risk_match = re.search(r'RiskScore:([\d\.]+)', line)
                        
                        if temp_match and risk_match:
                            t = float(temp_match.group(1))
                            r = float(risk_match.group(1))
                            nid = int(id_match.group(1)) if id_match else 1
                            
                            status = "NORMAL"
                            if r > 4.0: status = "CRITICAL"
                            
                            current_state = {
                                "id": nid, "temp": t, "risk": r, 
                                "status": status, "last_update": time.time()
                            }
                            print(f"LIVE DATA: {current_state}")
                    except ValueError:
                        pass
        except Exception as e:
            print(f"Serial Error: {e}")
            time.sleep(2)

threading.Thread(target=read_serial_loop, daemon=True).start()

@app.route('/data')
def get_data():
    # Force Offline if no data for 3 seconds
    if time.time() - current_state['last_update'] > 3:
        return jsonify({"id": 0, "temp": 0, "risk": 0, "status": "OFFLINE"})
    
    response = jsonify(current_state)
    # Add headers to force browser to accept it
    response.headers.add("Access-Control-Allow-Origin", "*")
    return response

if __name__ == '__main__':
    # Force IPv4 (127.0.0.1) instead of localhost
    print("🚀 Bridge Server Ready.")
    app.run(host='127.0.0.1', port=5000)
