import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { api } from '../api/client';

// UUIDs from ble_provisioning.cpp
const SERVICE_UUID = "f0debc9a-7856-3412-f0de-bc9a78563412";
const CHR_MAC = "e0debc9a-7856-3412-f0de-bc9a78563412";
const CHR_POP = "e1debc9a-7856-3412-f0de-bc9a78563412";
const CHR_SSID = "f1debc9a-7856-3412-f0de-bc9a78563412";
const CHR_PASS = "f2debc9a-7856-3412-f0de-bc9a78563412";
const CHR_STATUS = "f3debc9a-7856-3412-f0de-bc9a78563412";
const CHR_CMD = "f4debc9a-7856-3412-f0de-bc9a78563412";
const CHR_IDS = "f5debc9a-7856-3412-f0de-bc9a78563412";
const CHR_BROKER_URL = "f8debc9a-7856-3412-f0de-bc9a78563412";

export default function Provisioning() {
    const [device, setDevice] = useState(null);
    const [macAddress, setMacAddress] = useState('');
    const [pop, setPop] = useState('');
    const [ssid, setSsid] = useState('');
    const [password, setPassword] = useState('');
    const [customerId, setCustomerId] = useState('1'); // Defaulting to 1 as per backend schema
    const [locationId, setLocationId] = useState('');
    const [deviceName, setDeviceName] = useState('');
    const [brokerUrl, setBrokerUrl] = useState('');
    const [locations, setLocations] = useState([]);
    const [status, setStatus] = useState('Idle');
    const navigate = useNavigate();

    // Load locations and config on mount
    React.useEffect(() => {
        api.locations.list()
            .then(setLocations)
            .catch(err => console.error('Failed to load locations:', err));
            
        api.system.getConfig()
            .then(config => {
                if (config && config.mqttBrokerUrl) {
                    console.log("Fetched broker URL:", config.mqttBrokerUrl);
                    setBrokerUrl(config.mqttBrokerUrl);
                }
            })
            .catch(err => console.error('Failed to load system config:', err));
    }, []);

    const connect = async () => {
        try {
            if (!navigator.bluetooth) {
                alert("Bluetooth not supported. Use Bluefy browser on iOS or Chrome on Android.");
                return;
            }

            setStatus('Scanning...');
            const dev = await navigator.bluetooth.requestDevice({
                filters: [{ services: [SERVICE_UUID] }],
                optionalServices: [SERVICE_UUID]
            });

            const server = await dev.gatt.connect();
            setDevice(server);
            setStatus('Connected - Reading device info...');

            // Read MAC Address and PoP
            const service = await server.getPrimaryService(SERVICE_UUID);

            // Read MAC (6 bytes)
            const macChr = await service.getCharacteristic(CHR_MAC);
            const macValue = await macChr.readValue();
            const macBytes = new Uint8Array(macValue.buffer);
            const macStr = Array.from(macBytes).map(b => b.toString(16).padStart(2, '0').toUpperCase()).join(':');
            setMacAddress(macStr);

            // Read PoP (32 bytes)
            const popChr = await service.getCharacteristic(CHR_POP);
            const popValue = await popChr.readValue();
            const popBytes = new Uint8Array(popValue.buffer);
            const popHex = Array.from(popBytes).map(b => b.toString(16).padStart(2, '0')).join('');
            setPop(popHex);

            setStatus('Ready to Configure');
            console.log('Device MAC:', macStr);
            console.log('Device PoP:', popHex);
        } catch (e) {
            console.error(e);
            setStatus(`Error: ${e.message}`);
        }
    };

    const sendCreds = async () => {
        if (!device) return;
        if (!locationId || !deviceName) {
            setStatus('Please select location and enter device name');
            return;
        }

        try {
            setStatus('Writing WiFi Config...');
            const service = await device.getPrimaryService(SERVICE_UUID);

            const enc = new TextEncoder();

            // Write SSID
            const ssidChr = await service.getCharacteristic(CHR_SSID);
            await ssidChr.writeValue(enc.encode(ssid));

            // Write Password
            const passChr = await service.getCharacteristic(CHR_PASS);
            await passChr.writeValue(enc.encode(password));
            
            // Write Broker URL if available
            if (brokerUrl) {
                try {
                    console.log(`Writing Broker URL: ${brokerUrl}`);
                    const brokerChr = await service.getCharacteristic(CHR_BROKER_URL);
                    await brokerChr.writeValue(enc.encode(brokerUrl));
                } catch (e) {
                    console.error("Broker URL Write Failed", e);
                    // Continue even if this fails, as device might rely on default or previous
                }
            }

            // Write IDs
            const idsChr = await service.getCharacteristic(CHR_IDS);
            // Use MAC Address as the Device ID in the topic structure
            // This ensures the backend can look up the device by MAC address
            const idsPayload = `${customerId},${locationId},${macAddress}`;
            await idsChr.writeValue(enc.encode(idsPayload));

            // Write Save Command (0x01) - this will cause the device to disconnect and reboot
            const cmdChr = await service.getCharacteristic(CHR_CMD);
            await cmdChr.writeValue(new Uint8Array([0x01]));

            // The device will disconnect automatically. We can disconnect from the client-side too.
            if (device.connected) {
                device.disconnect();
            }

            setStatus('Config sent! Waiting for device to connect to WiFi...');

            // Wait for device to reboot and connect to WiFi
            await new Promise(resolve => setTimeout(resolve, 10000));

            // Claim device with backend
            try {
                setStatus('Claiming device with backend...');
                await api.devices.claim({
                    macAddress: macAddress,
                    proofOfPossession: pop,
                    locationId: parseInt(locationId),
                    name: deviceName
                });

                setStatus('Device claimed successfully!');
                setTimeout(() => {
                    navigate('/');
                }, 2000);
            } catch (claimError) {
                setStatus(`Claiming failed: ${claimError.message}`);
                console.error('Claim error:', claimError);
            }

        } catch (e) {
            console.error(e);
            // The disconnect might throw an error, which is expected.
            if (e.message.includes("disconnected")) {
                setStatus('Config sent! Waiting for device to connect to WiFi...');
                // Wait for device to reboot and connect to WiFi
                await new Promise(resolve => setTimeout(resolve, 10000));

                // Claim device with backend
                try {
                    setStatus('Claiming device with backend...');
                    await api.devices.claim({
                        macAddress: macAddress,
                        proofOfPossession: pop,
                        locationId: parseInt(locationId),
                        name: deviceName
                    });

                    setStatus('Device claimed successfully!');
                    setTimeout(() => {
                        navigate('/');
                    }, 2000);
                } catch (claimError) {
                    setStatus(`Claiming failed: ${claimError.message}`);
                    console.error('Claim error:', claimError);
                }
            } else {
                setStatus(`Error: ${e.message}`);
            }
        }
    };

    return (
        <div className="space-y-6">
            <h2 className="text-xl font-bold">Device Setup & Claiming v2 (MAC Fix)</h2>
            <div className="bg-yellow-50 p-4 rounded text-sm text-yellow-800">
                Ensure you are using <b>Bluefy</b> (iOS) or Chrome (Android/Desktop) with Bluetooth enabled.
            </div>

            {!device ? (
                <button onClick={connect} className="w-full bg-blue-600 text-white p-4 rounded-xl font-bold text-lg">
                    Scan for Device
                </button>
            ) : (
                <div className="space-y-4 animate-in fade-in">
                    <div className="text-green-600 font-bold text-center">Connected!</div>

                    {/* Device Info */}
                    {macAddress && (
                        <div className="bg-gray-100 p-3 rounded text-sm">
                            <div><strong>MAC:</strong> {macAddress}</div>
                            <div className="text-xs text-gray-600 break-all"><strong>PoP:</strong> {pop}</div>
                        </div>
                    )}

                    {/* WiFi Credentials */}
                    <div>
                        <label className="block text-sm font-bold">WiFi SSID</label>
                        <input className="w-full border p-2 rounded" value={ssid} onChange={e => setSsid(e.target.value)} />
                    </div>
                    <div>
                        <label className="block text-sm font-bold">WiFi Password</label>
                        <input className="w-full border p-2 rounded" type="password" value={password} onChange={e => setPassword(e.target.value)} />
                    </div>

                    {/* Customer ID (hidden for now, can be exposed if needed) */}
                    <input type="hidden" value={customerId} onChange={e => setCustomerId(e.target.value)} />

                    {/* Location Selection */}
                    <div>
                        <label className="block text-sm font-bold">Location</label>
                        <select className="w-full border p-2 rounded" value={locationId} onChange={e => setLocationId(e.target.value)}>
                            <option value="">Select location...</option>
                            {locations.map(loc => (
                                <option key={loc.id} value={loc.id}>{loc.name}</option>
                            ))}
                        </select>
                    </div>

                    {/* Device Name */}
                    <div>
                        <label className="block text-sm font-bold">Device Name</label>
                        <input className="w-full border p-2 rounded" value={deviceName} onChange={e => setDeviceName(e.target.value)} />
                    </div>

                    <button onClick={sendCreds} className="w-full bg-green-600 text-white p-4 rounded-xl font-bold text-lg">
                        Save &amp; Claim
                    </button>
                </div>
            )}

            <div className="text-center text-gray-500 font-mono text-sm">Status: {status}</div>
        </div>
    );
}
