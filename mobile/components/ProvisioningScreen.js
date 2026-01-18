import React, { useState, useEffect } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, Alert, ActivityIndicator, FlatList, Modal, ScrollView, Platform } from 'react-native';
import { encode, decode } from 'base-64';

// UUIDs matching C++ code (Reversed from byte array for Big Endian string format)
// Service: f0debc9a-7856-3412-f0de-bc9a78563412
const SERVICE_UUID = "f0debc9a-7856-3412-f0de-bc9a78563412";
const CHR_SSID = "f1debc9a-7856-3412-f0de-bc9a78563412";
const CHR_PASS = "f2debc9a-7856-3412-f0de-bc9a78563412";
const CHR_CMD = "f4debc9a-7856-3412-f0de-bc9a78563412"; // Write 0x01 to save
const CHR_IDS = "f5debc9a-7856-3412-f0de-bc9a78563412"; // Combined IDs
const CHR_MAC = "e0debc9a-7856-3412-f0de-bc9a78563412"; // Read MAC
const CHR_BROKER_URL = "f8debc9a-7856-3412-f0de-bc9a78563412"; // Broker URL

const delay = ms => new Promise(res => setTimeout(res, ms));

export default function ProvisioningScreen({ manager, host, onFinish, onCancel }) {
    const [ssid, setSsid] = useState('');
    const [password, setPassword] = useState('');
    const [devName, setDevName] = useState('');
    const [locationId, setLocationId] = useState('');
    const [customerId] = useState('1'); // Hidden default
    const [brokerUrl, setBrokerUrl] = useState('');

    const [status, setStatus] = useState('Idle');
    const [device, setDevice] = useState(null);
    const [isScanning, setIsScanning] = useState(false);
    const [scannedDevices, setScannedDevices] = useState([]);

    // Room logic
    const [locations, setLocations] = useState([]);
    const [showRoomModal, setShowRoomModal] = useState(false);
    const [newRoomName, setNewRoomName] = useState('');

    useEffect(() => {
        fetchLocations();
        fetchSystemConfig();
        return () => manager.stopDeviceScan();
    }, []);

    const fetchSystemConfig = async () => {
        try {
            console.log(`Fetching config from: ${host}/api/config`);
            const res = await fetch(`${host}/api/config`);
            if (res.ok) {
                const data = await res.json();
                console.log("System Config:", data);
                if (data.mqttBrokerUrl) {
                    setBrokerUrl(data.mqttBrokerUrl);
                }
            } else {
                console.warn("Failed to fetch system config:", res.status);
            }
        } catch (e) {
            console.warn("Error fetching system config:", e);
        }
    };

    const fetchLocations = async () => {
        try {
            const res = await fetch(`${host}/api/locations`);
            if (res.ok) {
                const data = await res.json();
                setLocations(data);
                if (data.length > 0 && !locationId) {
                    setLocationId(String(data[0].id));
                }
            }
        } catch (e) {
            console.log("Failed to fetch locations", e);
        }
    };

    const createRoom = async () => {
        if (!newRoomName.trim()) return;
        try {
            const res = await fetch(`${host}/api/locations`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ name: newRoomName })
            });
            if (res.ok) {
                const newLoc = await res.json();
                setLocations(prev => [...prev, newLoc]);
                setLocationId(String(newLoc.id));
                setNewRoomName('');
                setShowRoomModal(false);
                Alert.alert("Success", "Room created!");
            } else {
                Alert.alert("Error", "Failed to create room");
            }
        } catch (e) {
            Alert.alert("Error", e.message);
        }
    };

    const startScan = async () => {
        if (isScanning) return;

        setScannedDevices([]);
        setStatus('Scanning for LED_WiFi_Config...');
        setIsScanning(true);

        manager.startDeviceScan(null, null, (error, scannedDevice) => {
            if (error) {
                console.log(error);
                setStatus('Scan Error: ' + error.message);
                setIsScanning(false);
                return;
            }

            if (scannedDevice && scannedDevice.name) {
                setScannedDevices(prev => {
                    if (prev.find(d => d.id === scannedDevice.id)) return prev;
                    return [...prev, scannedDevice];
                });
            }
        });

        setTimeout(() => {
            manager.stopDeviceScan();
            setIsScanning(false);
            setStatus('Scan finished.');
        }, 10000);
    };

    const connectToDevice = async (dev) => {
        try {
            manager.stopDeviceScan();
            setIsScanning(false);
            setStatus(`Connecting to ${dev.name}...`);

            const connectedDevice = await dev.connect();

            // Android-specific: Refresh GATT Cache to prevent "Unknown Error" on changed characteristics
            if (Platform.OS === 'android') {
                try {
                    // ble-plx doesn't expose refreshGatt directly in some versions, 
                    // but usually calling discoverAllServicesAndCharacteristics forces a refresh if connect was fresh.
                    // However, if we suspect sticky cache:
                    // Note: react-native-ble-plx doesn't have explicit refreshGatt() on the Device object in standard types
                    // We rely on simple discovery.
                } catch (err) { }
            }

            setDevice(connectedDevice);

            setStatus('Discovering services...');
            // Wait a bit before discovery for stability
            await delay(500);
            await connectedDevice.discoverAllServicesAndCharacteristics();

            // Log all services/characteristics for debugging
            const services = await connectedDevice.services();
            console.log("Services discovered:", services.map(s => s.uuid));

            setStatus('Connected!');
        } catch (e) {
            setStatus('Connection failed: ' + e.message);
            setIsScanning(false);
        }
    };

    const delay = (ms) => new Promise(resolve => setTimeout(resolve, ms));

    const sendCreds = async () => {
        if (!device) return;
        setIsScanning(true); // Disable input
        setStatus('Writing Configuration...');

        let registeredDeviceId = "0";

        // 1. Get MAC from Device and Register with Backend
        try {
            setStatus('Reading Device MAC...');
            const char = await device.readCharacteristicForService(SERVICE_UUID, CHR_MAC);
            const rawMac = decode(char.value);

            // Convert binary string to Hex format (AA:BB:CC...)
            const macHex = rawMac.split('').map(c => c.charCodeAt(0).toString(16).padStart(2, '0').toUpperCase()).join(':');
            console.log("Read Device MAC:", macHex);

            setStatus(`Registering Device (${macHex})...`);
            console.log(`Registering to: ${host}/api/devices/claim`);

            const res = await fetch(`${host}/api/devices/claim`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    macAddress: macHex,
                    proofOfPossession: '123456',
                    name: devName || `Smart LED ${macHex.slice(-5)}`,
                    locationId: Number(locationId)
                })
            });

            if (!res.ok) {
                console.warn("Registration status:", res.status);
            } else {
                console.log("Device registered successfully");
                try {
                    const data = await res.json();
                    if (data && data.id) registeredDeviceId = String(data.id);
                    console.log("Got Device ID:", registeredDeviceId);
                } catch (jsonErr) {
                    console.warn("Failed to parse device ID from response", jsonErr);
                }
            }
        } catch (e) {
            console.error("Failed to read MAC or Register", e);
            // We do NOT block provisioning if this fails (e.g. maybe it's already registered or backend glitch)
            // But the user should know if it persists.
        }

        // Diagnostic Read
        try {
            console.log("Attempting diagnostic read of SSID...");
            const readChar = await device.readCharacteristicForService(SERVICE_UUID, CHR_SSID);
            console.log("Read success. Value base64:", readChar.value);
        } catch (e) {
            console.error("Msg: Diagnostic Read Failed", e);
            setStatus(`Read Error (Check Logs): ${e.message}`);
            Alert.alert("Connection Error", "Could not read from device. Try reconnecting.");
            setIsScanning(false);
            return;
        }

        try {
            if (ssid.length > 0) {
                console.log(`Writing SSID: ${ssid}`);
                await device.writeCharacteristicWithResponseForService(
                    SERVICE_UUID, CHR_SSID, encode(ssid)
                );
                setStatus('SSID written...');
                await delay(500);
            } else {
                console.log("SSID empty, skipping write");
            }
        } catch (e) {
            console.error("SSID Write Failed", e);
            setStatus(`Error writing SSID: ${e.message}`);
            Alert.alert("Error", "Failed to write SSID");
            setIsScanning(false);
            return;
        }

        try {
            console.log(`Writing Password: ${password || '<empty>'}`);
            await device.writeCharacteristicWithResponseForService(
                SERVICE_UUID, CHR_PASS, encode(password)
            );
            setStatus('Password written...');
            await delay(500);
        } catch (e) {
            console.error("Password Write Failed", e);
            setStatus(`Error writing Password: ${e.message}`);
            Alert.alert("Error", "Failed to write Password");
            setIsScanning(false);
            return;
        }

        try {
            const idsStr = `${customerId},${locationId},${registeredDeviceId}`;
            console.log(`Writing IDs: ${idsStr}`);
            const encodedIds = encode(idsStr);
            console.log(`Encoded IDs: ${encodedIds}`);

            await device.writeCharacteristicWithResponseForService(
                SERVICE_UUID, CHR_IDS, encodedIds
            );
            setStatus('IDs written...');
            await delay(500);
        } catch (e) {
            console.error("IDs Write Failed", e);
            setStatus(`Error writing IDs: ${e.message}`);
            Alert.alert("Error", "Failed to write IDs");
            setIsScanning(false);
            return;
        }

        if (brokerUrl) {
            try {
                console.log(`Writing Broker URL: ${brokerUrl}`);
                await device.writeCharacteristicWithResponseForService(
                    SERVICE_UUID, CHR_BROKER_URL, encode(brokerUrl)
                );
                setStatus('Broker URL written...');
                await delay(500);
            } catch (e) {
                console.error("Broker URL Write Failed", e);
                // We don't block if this fails, as device might fallback
                setStatus(`Warning: Broker URL write failed: ${e.message}`);
            }
        }

        try {
            console.log("Writing Save Command (0x01)...");
            // Use WithoutResponse to avoid waiting for ack (since device reboots immediately)
            await device.writeCharacteristicWithoutResponseForService(
                SERVICE_UUID, CHR_CMD, "AQ=="
            );
            console.log("Save command sent.");
        } catch (e) {
            console.log("Save command finished (likely disconnected)", e);
        }

        // Always succeed getting here
        setStatus('Sent! Device will reboot.');
        Alert.alert("Success", "Configuration sent to device.", [
            { text: "OK", onPress: onFinish }
        ]);
        setIsScanning(false);
    };

    const getRoomName = () => {
        const r = locations.find(l => String(l.id) === String(locationId));
        return r ? r.name : "Select Room";
    };

    return (
        <View style={styles.container}>
            <Text style={styles.title}>Add New Device</Text>

            {!device ? (
                <>
                    <View style={styles.scanArea}>
                        <TouchableOpacity style={styles.scanBtn} onPress={startScan} disabled={isScanning}>
                            {isScanning ? <ActivityIndicator color="#fff" /> : <Text style={styles.btnText}>Scan for Devices</Text>}
                        </TouchableOpacity>
                        <Text style={styles.status}>{status}</Text>
                    </View>

                    <Text style={styles.listHeader}>Found Devices:</Text>
                    <FlatList
                        data={scannedDevices}
                        keyExtractor={item => item.id}
                        renderItem={({ item }) => (
                            <TouchableOpacity style={styles.deviceItem} onPress={() => connectToDevice(item)}>
                                <Text style={styles.deviceName}>{item.name || 'Unknown'}</Text>
                                <Text style={styles.deviceId}>{item.id}</Text>
                            </TouchableOpacity>
                        )}
                        ListEmptyComponent={<Text style={styles.emptyText}>No devices found yet.</Text>}
                    />

                    <View style={styles.hintBox}>
                        <Text style={styles.hintText}>💡 Tip: Long-press the BOOT button on the ESP32 for 3s until LED flashes to enter pairing mode.</Text>
                    </View>
                </>
            ) : (
                <View style={styles.form}>
                    <Text style={styles.connectedText}>Connected to {device.name}</Text>

                    <Text style={styles.label}>WiFi SSID</Text>
                    <TextInput style={styles.input} value={ssid} onChangeText={setSsid} placeholder="My WiFi" autoCapitalize="none" />

                    <Text style={styles.label}>WiFi Password</Text>
                    <TextInput style={styles.input} value={password} onChangeText={setPassword} secureTextEntry placeholder="Password" />

                    <Text style={styles.label}>Device Name (Optional)</Text>
                    <TextInput style={styles.input} value={devName} onChangeText={setDevName} placeholder="e.g. Living Room LED" />

                    <Text style={styles.label}>Room / Location</Text>
                    <TouchableOpacity style={styles.pickerBtn} onPress={() => setShowRoomModal(true)}>
                        <Text style={styles.pickerText}>{getRoomName()}</Text>
                        <Text style={{ fontSize: 12, color: '#007AFF' }}>Change</Text>
                    </TouchableOpacity>

                    <Text style={styles.status}>{status}</Text>

                    <TouchableOpacity style={styles.saveBtn} onPress={sendCreds}>
                        <Text style={styles.btnText}>Save configuration</Text>
                    </TouchableOpacity>
                </View>
            )}

            <TouchableOpacity style={styles.link} onPress={onCancel}>
                <Text style={styles.linkText}>Cancel</Text>
            </TouchableOpacity>

            <Modal visible={showRoomModal} animationType="slide" transparent>
                <View style={styles.modalOverlay}>
                    <View style={styles.modalContent}>
                        <Text style={styles.modalTitle}>Select Room</Text>
                        <ScrollView style={{ maxHeight: 200, marginBottom: 20 }}>
                            {locations.map(l => (
                                <TouchableOpacity
                                    key={l.id}
                                    style={[styles.roomItem, String(l.id) === String(locationId) && styles.roomItemSelected]}
                                    onPress={() => { setLocationId(String(l.id)); setShowRoomModal(false); }}
                                >
                                    <Text style={styles.roomItemText}>{l.name}</Text>
                                    {String(l.id) === String(locationId) && <Text style={{ color: '#007AFF' }}>✓</Text>}
                                </TouchableOpacity>
                            ))}
                        </ScrollView>

                        <Text style={styles.label}>Or Create New Room</Text>
                        <View style={{ flexDirection: 'row', gap: 10, marginBottom: 20 }}>
                            <TextInput
                                style={[styles.input, { flex: 1, marginBottom: 0 }]}
                                placeholder="New Room Name"
                                value={newRoomName}
                                onChangeText={setNewRoomName}
                            />
                            <TouchableOpacity style={[styles.btn, { padding: 12, marginTop: 0 }]} onPress={createRoom}>
                                <Text style={styles.btnText}>Add</Text>
                            </TouchableOpacity>
                        </View>

                        <TouchableOpacity style={styles.link} onPress={() => setShowRoomModal(false)}>
                            <Text style={styles.linkText}>Close</Text>
                        </TouchableOpacity>
                    </View>
                </View>
            </Modal>
        </View>
    );
}

const styles = StyleSheet.create({
    container: { flex: 1, padding: 20, paddingTop: 60, backgroundColor: '#f2f2f7' },
    title: { fontSize: 28, fontWeight: 'bold', marginBottom: 20, color: '#000' },
    scanArea: { marginBottom: 20 },
    scanBtn: { backgroundColor: '#007AFF', padding: 15, borderRadius: 12, alignItems: 'center' },
    btnText: { color: '#fff', fontSize: 16, fontWeight: '600' },
    status: { marginTop: 10, textAlign: 'center', color: '#666' },

    listHeader: { fontSize: 18, fontWeight: '700', marginBottom: 10, color: '#333' },
    deviceItem: { backgroundColor: '#fff', padding: 15, borderRadius: 10, marginBottom: 10, flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
    deviceName: { fontSize: 16, fontWeight: '600' },
    deviceId: { fontSize: 12, color: '#999' },
    emptyText: { textAlign: 'center', color: '#999', marginTop: 20 },

    form: { backgroundColor: '#fff', padding: 20, borderRadius: 15 },
    connectedText: { fontSize: 18, fontWeight: 'bold', color: '#34C759', marginBottom: 20, textAlign: 'center' },
    label: { fontSize: 14, fontWeight: '600', marginBottom: 5, color: '#333' },
    input: { borderWidth: 1, borderColor: '#e5e5ea', borderRadius: 8, padding: 12, fontSize: 16, marginBottom: 15, backgroundColor: '#f9f9f9' },
    row: { flexDirection: 'row' },
    saveBtn: { backgroundColor: '#34C759', padding: 15, borderRadius: 12, alignItems: 'center', marginTop: 10 },
    pickerBtn: { borderWidth: 1, borderColor: '#e5e5ea', borderRadius: 8, padding: 12, backgroundColor: '#f9f9f9', marginBottom: 15, flexDirection: 'row', justifyContent: 'space-between' },
    pickerText: { fontSize: 16 },

    link: { marginTop: 20, marginBottom: 50, alignItems: 'center' },
    linkText: { color: '#007AFF', fontSize: 16 },
    hintBox: { marginTop: 20, padding: 15, backgroundColor: '#FFF8E1', borderRadius: 8 },
    hintText: { color: '#F57F17', fontSize: 14 },

    modalOverlay: { flex: 1, backgroundColor: 'rgba(0,0,0,0.5)', justifyContent: 'center', padding: 20 },
    modalContent: { backgroundColor: '#fff', borderRadius: 20, padding: 20 },
    modalTitle: { fontSize: 20, fontWeight: 'bold', marginBottom: 15, textAlign: 'center' },
    roomItem: { padding: 15, borderBottomWidth: 1, borderBottomColor: '#eee', flexDirection: 'row', justifyContent: 'space-between' },
    roomItemSelected: { backgroundColor: '#F0F9FF' },
    roomItemText: { fontSize: 16 },
    btn: { backgroundColor: '#007AFF', padding: 15, borderRadius: 12, alignItems: 'center' },
});
