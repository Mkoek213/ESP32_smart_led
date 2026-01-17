import React, { useState, useEffect } from 'react';
import { View, Text, Switch, StyleSheet, ScrollView, TextInput, TouchableOpacity } from 'react-native';

const DEFAULT_HOST = 'http://172.20.10.7:8080';

export default function DashboardScreen({ onNavigateProvision }) {
  const [host, setHost] = useState(DEFAULT_HOST);
  const [state, setState] = useState({ state: 'unknown' });
  const [telemetry, setTelemetry] = useState({});
  
  useEffect(() => {
    const timer = setInterval(pollData, 2000);
    pollData(); // Initial run
    return () => clearInterval(timer);
  }, [host]);

  const pollData = async () => {
    try {
        // Mock hardcoded IDs for now
        const statusRes = await fetch(`${host}/api/devices/1/status`);
        if (statusRes.ok) {
            const statusJson = await statusRes.json();
            setState(statusJson);
        }

        const telemRes = await fetch(`${host}/api/telemetry/latest?deviceId=1`);
        if (telemRes.ok) {
            const telemJson = await telemRes.json();
            setTelemetry(telemJson);
        }
    } catch (e) {
        console.log("Poll error", e.message);
    }
  };

  const toggleLed = async (val) => {
      try {
          // Send command to backend
          // We assume the backend accepts a generic command structure we used in web
          await fetch(`${host}/api/devices/1/command`, {
              method: 'POST',
              headers: {'Content-Type': 'application/json'},
              body: JSON.stringify({ type: 'SET_STATE', payload: { on: val } }) 
          });
          // Optimistic update
          setState(prev => ({ ...prev, on: val }));
      } catch (e) {
          alert("Failed to toggle");
      }
  };

  return (
    <ScrollView contentContainerStyle={styles.container}>
      <Text style={styles.header}>Smart LED Dashboard</Text>
      
      <View style={styles.card}>
          <Text style={styles.label}>Backend IP / Host</Text>
          <TextInput 
            style={styles.input} 
            value={host} 
            onChangeText={setHost} 
            autoCapitalize="none" 
            autoCorrect={false}
          />
      </View>

      <View style={styles.card}>
         <Text style={styles.subHeader}>Status: {state.state}</Text>
         <View style={styles.row}>
             <Text style={{fontSize: 16}}>LED Power</Text>
             <Switch value={!!state.on} onValueChange={toggleLed} />
         </View>
      </View>

      <View style={styles.card}>
          <Text style={styles.subHeader}>Telemetry</Text>
          <View style={styles.statRow}><Text>Temperature:</Text><Text>{telemetry.temperature ? telemetry.temperature.toFixed(1) : '--'}°C</Text></View>
          <View style={styles.statRow}><Text>Humidity:</Text><Text>{telemetry.humidity ? telemetry.humidity.toFixed(1) : '--'}%</Text></View>
          <View style={styles.statRow}><Text>Brightness:</Text><Text>{telemetry.brightness || '--'}</Text></View>
          <View style={styles.statRow}><Text>Motion:</Text><Text>{telemetry.motionDetected ? 'Detected' : 'None'}</Text></View>
      </View>

      <TouchableOpacity style={styles.btn} onPress={onNavigateProvision}>
          <Text style={styles.btnText}>Configure New Device (Bluetooth)</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
    container: { padding: 20, paddingTop: 60, backgroundColor: '#f2f2f7', minHeight: '100%' },
    header: { fontSize: 32, fontWeight: 'bold', marginBottom: 20, color: '#000' },
    subHeader: { fontSize: 18, fontWeight: '600', marginBottom: 10, color: '#000' },
    card: { backgroundColor: '#fff', padding: 15, borderRadius: 12, marginBottom: 20, shadowColor:'#000', shadowOpacity: 0.1, shadowRadius:5, elevation: 2 },
    input: { borderWidth: 1, borderColor: '#eee', padding: 10, borderRadius: 8, marginTop: 5, fontSize: 16 },
    row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginTop: 10 },
    statRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
    label: { fontSize: 12, color: '#666', textTransform: 'uppercase', fontWeight: '600' },
    btn: { backgroundColor: '#007AFF', padding: 15, borderRadius: 12, alignItems: 'center', marginTop: 20 },
    btnText: { color: '#fff', fontWeight: 'bold', fontSize: 16 }
});
