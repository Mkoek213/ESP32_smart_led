import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';

// UUIDs from ble_provisioning.cpp
const SERVICE_UUID = "12345678-9abc-def0-1234-56789abcdef0";
const CHR_SSID = "12345678-9abc-def0-1234-56789abcdef1";
const CHR_PASS = "12345678-9abc-def0-1234-56789abcdef2";
const CHR_CMD  = "12345678-9abc-def0-1234-56789abcdef4"; // Write 0x01 to save

export default function Provisioning() {
  const [device, setDevice] = useState(null);
  const [ssid, setSsid] = useState('');
  const [password, setPassword] = useState('');
  const [status, setStatus] = useState('Idle');
  const navigate = useNavigate();

  const connect = async () => {
    try {
        if (!navigator.bluetooth) {
            alert("Bluetooth not supported. Use Bluefy browser on iOS.");
            return;
        }

        setStatus('Scanning...');
        const dev = await navigator.bluetooth.requestDevice({
            filters: [{ name: 'LED_WiFi_Config' }],
            optionalServices: [SERVICE_UUID]
        });

        const server = await dev.gatt.connect();
        setDevice(server);
        setStatus('Connected');
    } catch (e) {
        console.error(e);
        setStatus(`Error: ${e.message}`);
    }
  };

  const sendCreds = async () => {
    if (!device) return;
    try {
        setStatus('Writing Config...');
        const service = await device.getPrimaryService(SERVICE_UUID);
        
        const enc = new TextEncoder();

        // Write SSID
        const ssidChr = await service.getCharacteristic(CHR_SSID);
        await ssidChr.writeValue(enc.encode(ssid));

        // Write Password
        const passChr = await service.getCharacteristic(CHR_PASS);
        await passChr.writeValue(enc.encode(password));

        // Write Save Command (0x01)
        const cmdChr = await service.getCharacteristic(CHR_CMD);
        await cmdChr.writeValue(new Uint8Array([0x01]));

        setStatus('Sent! Device should reboot.');
        setTimeout(() => {
            if (device.connected) device.disconnect();
            navigate('/');
        }, 2000);

    } catch (e) {
        console.error(e);
        setStatus(`Write Error: ${e.message}`);
    }
  };

  return (
    <div className="space-y-6">
        <h2 className="text-xl font-bold">WiFi Setup</h2>
        <div className="bg-yellow-50 p-4 rounded text-sm text-yellow-800">
            Ensure you are using <b>Bluefy</b> (iOS) or Chrome (Android/Desktop) with Bluetooth enabled.
        </div>

        {!device ? (
            <button onClick={connect} className="w-full bg-blue-600 text-white p-4 rounded-xl font-bold text-lg">
                Scan for Device
            </button>
        ) : (
            <div className="space-y-4 animate-in fade-in">
                <div className="text-green-600 font-bold text-center">Connected to LED Config!</div>
                
                <div>
                   <label className="block text-sm font-bold">WiFi SSID</label>
                   <input className="w-full border p-2 rounded" value={ssid} onChange={e => setSsid(e.target.value)} />
                </div>
                <div>
                   <label className="block text-sm font-bold">WiFi Password</label>
                   <input className="w-full border p-2 rounded" type="password" value={password} onChange={e => setPassword(e.target.value)} />
                </div>

                <button onClick={sendCreds} className="w-full bg-green-600 text-white p-3 rounded font-bold">
                    Save to Device
                </button>
            </div>
        )}

        <div className="text-center text-gray-500 font-mono text-sm">Status: {status}</div>
    </div>
  );
}
