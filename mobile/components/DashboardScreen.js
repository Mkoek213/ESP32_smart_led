import React, { useState, useEffect } from 'react';
import { View, Text, Switch, StyleSheet, ScrollView, TextInput, TouchableOpacity, RefreshControl, FlatList, Modal, Alert } from 'react-native';

export default function DashboardScreen({ host, setHost, onNavigateProvision, onNavigateSettings }) {
  const [viewMode, setViewMode] = useState('list'); // 'list' | 'detail'
  
  // List Mode Data
  const [devices, setDevices] = useState([]);
  const [locations, setLocations] = useState([]);
  const [selectedRoomId, setSelectedRoomId] = useState(null); // null = All
  const [showHostInput, setShowHostInput] = useState(false); // For editing IP in List view

  // Detail Mode Data
  const [selectedDevice, setSelectedDevice] = useState(null);
  const [deviceState, setDeviceState] = useState({ on: false }); // Local state for immediate feedback
  const [telemetry, setTelemetry] = useState({});
  
  const [refreshing, setRefreshing] = useState(false);
  const [showAddRoom, setShowAddRoom] = useState(false);
  const [newRoomName, setNewRoomName] = useState('');

  useEffect(() => {
    fetchData(); 
  }, [host]);

  // Poll telemetry if in detail mode
  useEffect(() => {
      let timer;
      if (viewMode === 'detail' && selectedDevice) {
          fetchDeviceDetail(); // Initial fetch
          timer = setInterval(fetchDeviceDetail, 2000);
      }
      return () => clearInterval(timer);
  }, [viewMode, selectedDevice]);

  const fetchData = async () => {
    setRefreshing(true);
    try {
        const [devRes, locRes] = await Promise.all([
            fetch(`${host}/api/devices`),
            fetch(`${host}/api/locations`)
        ]);

        if (devRes.ok) {
            const devs = await devRes.json();
            setDevices(devs);
        }
        if (locRes.ok) {
            const locs = await locRes.json();
            setLocations(locs);
        }
    } catch (e) {
        console.log("Fetch error", e.message);
    }
    setRefreshing(false);
  };

  const fetchDeviceDetail = async () => {
    if (!selectedDevice) return;
    try {
        // 1. Get Device Info (inc status string)
        const devRes = await fetch(`${host}/api/devices/${selectedDevice.id}`);
        if(devRes.ok) {
            await devRes.json();
        }

        // 2. Get Telemetry
        const telemUrl = `${host}/api/telemetry/${selectedDevice.id}/latest`;
        console.log(`[Dashboard] Fetching telemetry: ${telemUrl}`);
        const telemRes = await fetch(telemUrl);

        if (telemRes.ok) {
            const text = await telemRes.text();
            console.log(`[Dashboard] Telemetry response: ${text}`);
            if (text) {
                setTelemetry(JSON.parse(text));
            } else {
                 setTelemetry({});
            }
        } else {
             console.log(`[Dashboard] Telemetry fetch failed: ${telemRes.status}`);
        }
    } catch (e) {
        console.log("Poll detail error", e);
    }
  };

  const createRoom = async () => {
      if (!newRoomName.trim()) return;
      try {
          const res = await fetch(`${host}/api/locations`, {
              method: 'POST',
              headers: {'Content-Type': 'application/json'},
              body: JSON.stringify({ name: newRoomName })
          });
          if (res.ok) {
              fetchData();
              setNewRoomName('');
              setShowAddRoom(false);
          } else {
              Alert.alert("Error", "Failed to create room");
          }
      } catch (e) {
          Alert.alert("Error", e.message);
      }
  };

  const deleteRoom = async (id) => {
    Alert.alert("Delete Room", "Are you sure?", [
        { text: "Cancel" },
        { text: "Delete", style:'destructive', onPress: async () => {
            try {
                await fetch(`${host}/api/locations/${id}`, { method: 'DELETE' });
                fetchData();
            } catch(e) { Alert.alert("Error", e.message); }
        }}
    ]);
  };

  const deleteDevice = async () => {
    if (!selectedDevice) return;
    Alert.alert("Delete Device", `Are you sure you want to delete ${selectedDevice.name}?`, [
        { text: "Cancel" },
        { text: "Delete", style: 'destructive', onPress: async () => {
             try {
                await fetch(`${host}/api/devices/${selectedDevice.id}`, { method: 'DELETE' });
                setViewMode('list');
                fetchData();
             } catch (e) {
                 Alert.alert("Error", e.message);
             }
        }}
    ]);
  };

  const toggleLed = async (val) => {
      if (!selectedDevice) return;
      try {
          setDeviceState(prev => ({ ...prev, on: val })); // Optimistic UI
          await fetch(`${host}/api/devices/${selectedDevice.id}/control`, {
              method: 'POST',
              headers: {'Content-Type': 'application/json'},
              body: JSON.stringify({ type: 'SET_STATE', payload: { on: val } }) 
          });
      } catch (e) {
          Alert.alert("Error", "Failed to send command");
      }
  };

  const getFilteredDevices = () => {
      if (!selectedRoomId) return devices;
      return devices.filter(d => String(d.locationId) === String(selectedRoomId));
  };

  const openDevice = (device) => {
      setSelectedDevice(device);
      setViewMode('detail');
  };

  // UI Components
  const Card = ({ title, value, unit, colorBg, colorText }) => (
    <View style={[styles.miniCard, { backgroundColor: colorBg }]}>
        <Text style={[styles.miniCardLabel, { color: colorText }]}>{title}</Text>
        <Text style={[styles.miniCardValue, { color: colorText }]}>
            {value !== undefined && value !== null ? value : '--'}
            <Text style={{fontSize: 14}}>{unit}</Text>
        </Text>
    </View>
  );

  // RENDER: LIST MODE
  if (viewMode === 'list') {
      return (
        <View style={styles.container}>
            <View style={styles.header}>
                <Text style={styles.headerTitle}>My Devices</Text>
                <TouchableOpacity style={styles.settingsBtn} onPress={() => setShowHostInput(!showHostInput)}>
                    <Text style={styles.settingsBtnText}>{showHostInput ? '▲' : '▼'}</Text>
                </TouchableOpacity>
            </View>

            {showHostInput && (
                <View style={[styles.debugCard, {marginBottom:15}]}>
                     <Text style={styles.label}>Backend IP</Text>
                     <TextInput 
                        style={styles.input} 
                        value={host} 
                        onChangeText={setHost} 
                        autoCapitalize="none"
                     />
                </View>
            )}

            {/* Room Filters */}
            <View style={{marginBottom: 15, flexDirection:'row', alignItems:'center'}}>
                <ScrollView horizontal showsHorizontalScrollIndicator={false} style={{flexGrow:0}}>
                    <TouchableOpacity 
                        style={[styles.roomPill, !selectedRoomId && styles.roomPillActive]} 
                        onPress={() => setSelectedRoomId(null)}
                    >
                        <Text style={[styles.roomPillText, !selectedRoomId && styles.roomPillTextActive]}>All</Text>
                    </TouchableOpacity>
                    {locations.map(loc => (
                        <TouchableOpacity 
                            key={loc.id} 
                            style={[styles.roomPill, selectedRoomId === loc.id && styles.roomPillActive]} 
                            onPress={() => setSelectedRoomId(loc.id)}
                            onLongPress={() => deleteRoom(loc.id)}
                        >
                            <Text style={[styles.roomPillText, selectedRoomId === loc.id && styles.roomPillTextActive]}>{loc.name}</Text>
                        </TouchableOpacity>
                    ))}
                </ScrollView>
                <TouchableOpacity style={styles.addRoomBtn} onPress={() => setShowAddRoom(true)}>
                    <Text style={{color:'#fff', fontWeight:'bold'}}>+</Text>
                </TouchableOpacity>
            </View>

            {/* Device List */}
            <FlatList
                data={getFilteredDevices()}
                keyExtractor={item => String(item.id)}
                refreshControl={<RefreshControl refreshing={refreshing} onRefresh={fetchData} />}
                ListEmptyComponent={<Text style={{textAlign:'center', color:'#999', marginTop: 50}}>No devices found. Add one!</Text>}
                renderItem={({item}) => (
                    <TouchableOpacity style={styles.deviceCard} onPress={() => openDevice(item)}>
                        <View>
                            <Text style={styles.deviceCardName}>{item.name}</Text>
                            <Text style={styles.deviceCardLoc}>
                                {locations.find(l => l.id === item.locationId)?.name || 'Unknown Room'}
                            </Text>
                        </View>
                        <View style={{flexDirection:'row', alignItems:'center', gap:10}}>
                             <View style={[styles.statusDot, {backgroundColor: item.status === 'ONLINE' ? '#34C759' : '#8E8E93'}]} />
                             <Text style={{color:'#ccc'}}>›</Text>
                        </View>
                    </TouchableOpacity>
                )}
            />

            <TouchableOpacity style={styles.fab} onPress={onNavigateProvision}>
                <Text style={styles.fabText}>+ Device</Text>
            </TouchableOpacity>

            {/* Add Room Modal */}
            <Modal visible={showAddRoom} transparent animationType="fade">
                <View style={styles.modalOverlay}>
                    <View style={styles.modalContent}>
                        <Text style={styles.modalTitle}>Add New Room</Text>
                        <TextInput 
                            style={styles.input} 
                            placeholder="Room Name" 
                            value={newRoomName} 
                            onChangeText={setNewRoomName} 
                        />
                        <View style={{flexDirection:'row', justifyContent:'flex-end', gap:10}}>
                            <TouchableOpacity onPress={() => setShowAddRoom(false)}><Text style={{padding:10, color:'#666'}}>Cancel</Text></TouchableOpacity>
                            <TouchableOpacity onPress={createRoom}><Text style={{padding:10, color:'#007AFF', fontWeight:'bold'}}>Create</Text></TouchableOpacity>
                        </View>
                    </View>
                </View>
            </Modal>
        </View>
      );
  }

  // RENDER: DETAIL MODE
  return (
    <ScrollView 
        style={styles.container} 
        contentContainerStyle={{paddingBottom: 40}}
    >
      <View style={styles.header}>
        <TouchableOpacity onPress={() => setViewMode('list')} style={{marginRight:10}}>
            <Text style={{fontSize:24}}>‹</Text>
        </TouchableOpacity>
        <Text style={styles.headerTitle}>{selectedDevice?.name || 'Device'}</Text>
        <View style={{flexDirection:'row', gap: 10}}>
             <TouchableOpacity style={{padding:5}} onPress={() => onNavigateSettings(selectedDevice.id)}>
                <Text style={{fontSize:22}}>⚙️</Text>
             </TouchableOpacity>
             <TouchableOpacity style={{padding:5}} onPress={deleteDevice}>
                <Text style={{fontSize:22}}>🗑️</Text>
             </TouchableOpacity>
        </View>
      </View>

      {/* Main Control Card */}
      <View style={styles.mainCard}>
          <View>
            <Text style={styles.deviceStatus}>
                Status: {deviceState.on ? <Text style={{color:'#34C759'}}>Active</Text> : <Text style={{color:'#8E8E93'}}>Standby</Text>}
            </Text>
            <Text style={styles.deviceName}>Smart LED Strip</Text>
          </View>
          <Switch 
            value={!!deviceState.on} 
            onValueChange={toggleLed}
            trackColor={{ false: "#767577", true: "#34C759" }}
            thumbColor={"#f4f3f4"}
            style={{ transform: [{ scaleX: 1.2 }, { scaleY: 1.2 }] }}
          />
      </View>
        
      <Text style={styles.sectionTitle}>Environment</Text>
      
      <View style={styles.grid}>
          <Card 
            title="Temperature" 
            value={telemetry.temperature ? telemetry.temperature.toFixed(1) : '--'} 
            unit="°C" 
            colorBg="#E3F2FD" 
            colorText="#1565C0" 
          />
          <Card 
            title="Humidity" 
            value={telemetry.humidity ? telemetry.humidity.toFixed(1) : '--'} 
            unit="%" 
            colorBg="#E8F5E9" 
            colorText="#2E7D32" 
          />
          <Card 
            title="Pressure" 
            value={telemetry.pressure ? telemetry.pressure.toFixed(0) : '--'} 
            unit="hPa" 
            colorBg="#FFF8E1" 
            colorText="#F9A825" 
          />
          <Card 
            title="Motion" 
            value={telemetry.personCount > 0 ? 'Detected' : 'None'} 
            unit="" 
            colorBg="#F3E5F5" 
            colorText="#7B1FA2" 
          />
      </View>

      <Text style={[styles.sectionTitle, {marginTop: 20}]}>Debug Info</Text>
      <View style={styles.debugCard}>
          <Text style={styles.label}>Device ID: {selectedDevice?.id}</Text>
          <Text style={styles.label}>Backend: {host}</Text>
          <Text style={styles.lastUpdate}>
            Last Update: {telemetry.timestamp ? new Date(telemetry.timestamp).toLocaleString() : 'Never'}
          </Text>
      </View>

    </ScrollView>
  );
}

const styles = StyleSheet.create({
    container: { flex: 1, backgroundColor: '#F2F2F7', padding: 20, paddingTop: 60 },
    header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 20 },
    headerTitle: { fontSize: 28, fontWeight: '800', color: '#000' },
    settingsBtn: { backgroundColor: '#E5E5EA', width: 40, height: 40, borderRadius: 20, justifyContent: 'center', alignItems: 'center' },
    settingsBtnText: { fontSize: 20 },
    
    // List Styles
    roomPill: { backgroundColor: '#fff', paddingHorizontal: 16, paddingVertical: 8, borderRadius: 20, marginRight: 8, height: 36, justifyContent:'center' },
    roomPillActive: { backgroundColor: '#000' },
    roomPillText: { fontWeight: '600', color: '#333' },
    roomPillTextActive: { color: '#fff' },
    addRoomBtn: { width: 36, height: 36, borderRadius: 18, backgroundColor: '#007AFF', justifyContent:'center', alignItems:'center', marginLeft: 5 },

    deviceCard: { backgroundColor: '#fff', padding: 20, borderRadius: 16, marginBottom: 12, flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', shadowColor: "#000", shadowOffset: { width: 0, height: 1 }, shadowOpacity: 0.05, shadowRadius: 3 },
    deviceCardName: { fontSize: 18, fontWeight: '700', color: '#333' },
    deviceCardLoc: { fontSize: 13, color: '#999', marginTop: 2 },
    statusDot: { width: 10, height: 10, borderRadius: 5 },

    // FAB
    fab: { position: 'absolute', bottom: 30, right: 20, backgroundColor: '#007AFF', paddingHorizontal: 20, paddingVertical: 15, borderRadius: 30, shadowColor: "#000", shadowOpacity: 0.3, shadowOffset: {width:0, height:4}, elevation: 5 },
    fabText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },

    // Modal
    modalOverlay: { flex:1, backgroundColor:'rgba(0,0,0,0.5)', justifyContent:'center', padding:40 },
    modalContent: { backgroundColor:'#fff', padding:20, borderRadius:15 },
    modalTitle: { fontSize:18, fontWeight:'bold', marginBottom:15 },
    
    // Detail Styles
    mainCard: { 
        backgroundColor: '#fff', 
        borderRadius: 20, 
        padding: 20, 
        flexDirection: 'row', 
        justifyContent: 'space-between', 
        alignItems: 'center',
        shadowColor: "#000", shadowOffset: { width: 0, height: 2 }, shadowOpacity: 0.1, shadowRadius: 8, elevation: 4,
        marginBottom: 25
    },
    deviceName: { fontSize: 22, fontWeight: '700', color: '#000', marginTop: 4 },
    deviceStatus: { fontSize: 14, fontWeight: '600', color: '#666', textTransform: 'uppercase' },

    sectionTitle: { fontSize: 18, fontWeight: '700', color: '#333', marginBottom: 15, marginLeft: 4 },
    grid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
    miniCard: { 
        width: '48%', 
        borderRadius: 16, 
        padding: 15, 
        marginBottom: 15, 
        justifyContent: 'space-between',
        height: 100
    },
    miniCardLabel: { fontSize: 14, fontWeight: '600', marginBottom: 5 },
    miniCardValue: { fontSize: 24, fontWeight: 'bold' },

    debugCard: { backgroundColor: '#fff', borderRadius: 12, padding: 15, marginBottom: 20 },
    label: { fontSize: 12, textTransform: 'uppercase', color: '#8E8E93', marginBottom: 5, fontWeight: '600' },
    input: { fontSize: 16, borderBottomWidth: 1, borderColor: '#eee', paddingVertical: 5, color: '#007AFF' },
    lastUpdate: { marginTop: 10, fontSize: 12, color: '#aaa', textAlign: 'right'},
});
