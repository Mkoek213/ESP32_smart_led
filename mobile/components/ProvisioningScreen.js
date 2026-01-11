import React, { useState, useEffect } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, Alert, ActivityIndicator } from 'react-native';
import { encode } from 'base-64';

// UUIDs
const SERVICE_UUID = "12345678-9abc-def0-1234-56789abcdef0";
const CHR_SSID = "12345678-9abc-def0-1234-56789abcdef1";
const CHR_PASS = "12345678-9abc-def0-1234-56789abcdef2";
const CHR_CMD  = "12345678-9abc-def0-1234-56789abcdef4"; // Write 0x01 to save

export default function ProvisioningScreen({ manager, onFinish, onCancel }) {
  const [ssid, setSsid] = useState('');
  const [password, setPassword] = useState('');
  const [status, setStatus] = useState('Idle');
  const [device, setDevice] = useState(null);
  const [isScanning, setIsScanning] = useState(false);

  useEffect(() => {
    // Cleanup on unmount
    return () => {
      manager.stopDeviceScan();
    };
  }, []);

  const startScan = async () => {
    if (isScanning) return;
    
    // Check permissions (Assume handled by App or request here)
    // On Android we need fine location
    
    setStatus('Scanning...');
    setIsScanning(true);
    
    manager.startDeviceScan(null, null, (error, scannedDevice) => {
      if (error) {
        console.log(error);
        setStatus('Error: ' + error.message);
        setIsScanning(false);
        return;
      }

      if (scannedDevice.name === 'LED_WiFi_Config') {
        manager.stopDeviceScan();
        setIsScanning(false);
        setStatus('Found device. Connecting...');
        connectToDevice(scannedDevice);
      }
    });

    // Timeout after 15s
    setTimeout(() => {
        // We can't access isScanning in closure reliably without ref, but manager call is safe
        manager.stopDeviceScan();
        setIsScanning(false); // This might toggle back if mounted
        if (!device) setStatus((prev) => prev.startsWith('Found') ? prev : 'Scan timeout. Try again.');
    }, 15000);
  };

  const connectToDevice = async (dev) => {
    try {
        const connectedDevice = await dev.connect();
        setDevice(connectedDevice);
        setStatus('Discovering services...');
        await connectedDevice.discoverAllServicesAndCharacteristics();
        setStatus('Connected!');
    } catch (e) {
        setStatus('Connection failed: ' + e.message);
        setIsScanning(false);
    }
  };

  const sendCreds = async () => {
    if (!device) return;
    try {
        setStatus('Writing Config...');
        
        // Write SSID
        await device.writeCharacteristicWithResponseForService(
            SERVICE_UUID,
            CHR_SSID,
            encode(ssid)
        );

        // Write Password
        await device.writeCharacteristicWithResponseForService(
            SERVICE_UUID,
            CHR_PASS,
            encode(password)
        );

        // Write Save Command (0x01)
        // 0x01 in base64 is AQ==
        await device.writeCharacteristicWithResponseForService(
            SERVICE_UUID,
            CHR_CMD,
            "AQ==" 
        );

        setStatus('Sent! Device should reboot.');
        Alert.alert("Success", "Configuration sent to device.", [
            { text: "OK", onPress: onFinish }
        ]);

    } catch (e) {
        console.error(e);
        setStatus(`Write Error: ${e.message}`);
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>WiFi Setup</Text>
      
      <Text style={styles.status}>{status}</Text>

      {!device ? (
        <TouchableOpacity style={styles.btn} onPress={startScan} disabled={isScanning}>
            {isScanning ? <ActivityIndicator color="#fff"/> : <Text style={styles.btnText}>Scan for Device</Text>}
        </TouchableOpacity>
      ) : (
        <View style={styles.form}>
            <Text style={styles.label}>WiFi SSID</Text>
            <TextInput style={styles.input} value={ssid} onChangeText={setSsid} placeholder="My WiFi" autoCapitalize="none" />
            
            <Text style={styles.label}>WiFi Password</Text>
            <TextInput style={styles.input} value={password} onChangeText={setPassword} secureTextEntry placeholder="Secret" />
            
            <TouchableOpacity style={[styles.btn, styles.btnGreen]} onPress={sendCreds}>
                <Text style={styles.btnText}>Save to Device</Text>
            </TouchableOpacity>
        </View>
      )}
      
      <TouchableOpacity style={styles.link} onPress={onCancel}>
          <Text style={styles.linkText}>Back to Dashboard</Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
    container: { flex: 1, padding: 20, justifyContent: 'center', backgroundColor: '#f2f2f7' },
    title: { fontSize: 28, fontWeight: 'bold', marginBottom: 20, textAlign: 'center', color: '#000' },
    status: { fontSize: 16, marginBottom: 20, textAlign: 'center', color: '#666' },
    btn: { backgroundColor: '#007AFF', padding: 15, borderRadius: 12, alignItems: 'center' },
    btnGreen: { backgroundColor: '#34C759', marginTop: 10 },
    btnText: { color: '#fff', fontSize: 18, fontWeight: '600' },
    form: { gap: 15 },
    label: { fontSize: 16, fontWeight: '600', marginLeft: 4, marginBottom: 4 },
    input: { borderWidth: 1, borderColor: '#e5e5ea', borderRadius: 10, padding: 12, fontSize: 16, backgroundColor: '#fff' },
    link: { marginTop: 30, alignItems: 'center' },
    linkText: { color: '#007AFF', fontSize: 16 }
});
