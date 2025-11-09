#!/usr/bin/env python3
"""
Simple HTTP test server for ESP32 HTTP client testing
Run this on your computer and point ESP32 to your local IP
"""

from flask import Flask, request
import datetime

app = Flask(__name__)

@app.route('/')
def home():
    """Simple HTML homepage"""
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    client_ip = request.remote_addr
    
    html = f"""<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Test Server</title>
    <style>
        body {{
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background-color: #f0f0f0;
        }}
        .container {{
            background-color: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }}
        h1 {{
            color: #333;
        }}
        .info {{
            background-color: #e8f4f8;
            padding: 15px;
            border-left: 4px solid #2196F3;
            margin: 20px 0;
        }}
        .success {{
            color: #4CAF50;
            font-weight: bold;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 HTTP Client Test Server</h1>
        <div class="info">
            <p class="success">Connection Successful!</p>
            <p><strong>Server Time:</strong> {timestamp}</p>
            <p><strong>Client IP:</strong> {client_ip}</p>
            <p><strong>Request Method:</strong> {request.method}</p>
        </div>
        <h2>Test Data</h2>
        <p>This is a test HTML page served from a Flask server.</p>
        <p>Your ESP32 successfully made an HTTP GET request!</p>
        <ul>
            <li>WiFi Connection: OK</li>
            <li>DNS Resolution: OK</li>
            <li>TCP Connection: OK</li>
            <li>HTTP GET Request: OK</li>
        </ul>
    </div>
</body>
</html>"""
    return html

@app.route('/test')
def test():
    """Simple test endpoint"""
    return "ESP32 Test - Hello from Flask Server!"

@app.route('/json')
def json_data():
    """JSON endpoint"""
    return {
        "message": "Hello ESP32!",
        "timestamp": datetime.datetime.now().isoformat(),
        "status": "success"
    }

@app.route('/sensor')
def sensor():
    """Simulate sensor data"""
    return {
        "temperature": 23.5,
        "humidity": 65.3,
        "pressure": 1013.25,
        "timestamp": datetime.datetime.now().isoformat()
    }

if __name__ == '__main__':
    print("=" * 60)
    print("ESP32 HTTP Test Server Starting...")
    print("=" * 60)
    print("\nAvailable endpoints:")
    print("  http://YOUR_IP:5000/        - Main page (HTML)")
    print("  http://YOUR_IP:5000/test    - Simple text response")
    print("  http://YOUR_IP:5000/json    - JSON response")
    print("  http://YOUR_IP:5000/sensor  - Simulated sensor data")
    print("\nTo find your IP address:")
    print("  Linux: ip addr show | grep 'inet ' | grep -v 127.0.0.1")
    print("  Or check your network settings")
    print("=" * 60)
    
    # Run on all interfaces so ESP32 can access it
    app.run(host='0.0.0.0', port=80, debug=True)

