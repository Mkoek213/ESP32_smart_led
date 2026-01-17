import React, { useState } from 'react';
import { View, Text, TextInput, StyleSheet, ScrollView, Switch, TouchableOpacity, Alert, Modal } from 'react-native';

const PRESET_COLORS = [
    "FF0000", "00FF00", "0000FF", // RGB
    "FFFFFF", "FFFF00", "00FFFF", // CMY/White
    "FFA500", "800080", "FFC0CB", // Orange, Purple, Pink
    "8B4513", "4B0082", "1E90FF"  // Brown, Indigo, DodgerBlue
];

export default function SettingsScreen({ onBack, host, deviceId }) {
    const [config, setConfig] = useState({
        colors: ["FF0000", "00FF00", "0000FF"],
        humidityThresholds: ["30.0", "50.0", "70.0", "90.0"],
        numLeds: "5",
        autobrightness: true,
        brightnessPct: "80",
        distanceThresholdCm: "50.0"
    });

    const [pickerVisible, setPickerVisible] = useState(false);
    const [activeColorIndex, setActiveColorIndex] = useState(null);

    const openColorPicker = (index) => {
        setActiveColorIndex(index);
        setPickerVisible(true);
    };

    const selectColor = (hex) => {
        if (activeColorIndex !== null) {
            const newColors = [...config.colors];
            newColors[activeColorIndex] = hex;
            setConfig({ ...config, colors: newColors });
        }
        setPickerVisible(false);
    };

    const updateColorManual = (index, val) => {
        const newColors = [...config.colors];
        newColors[index] = val;
        setConfig({ ...config, colors: newColors });
    };

    const updateThres = (index, val) => {
        const newThres = [...config.humidityThresholds];
        newThres[index] = val;
        setConfig({ ...config, humidityThresholds: newThres });
    };

    const saveConfig = async () => {
        try {
            const payload = {
                colors: config.colors,
                humidityThresholds: config.humidityThresholds.map(parseFloat),
                numLeds: parseInt(config.numLeds),
                autobrightness: config.autobrightness,
                brightnessPct: parseInt(config.brightnessPct),
                distanceThresholdCm: parseFloat(config.distanceThresholdCm)
            };

            const res = await fetch(`${host}/api/devices/${deviceId}/control`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ type: 'SET_CONFIG', payload })
            });

            if (res.ok) {
                Alert.alert("Success", "Configuration sent to device!");
                onBack(); // Go back on success? Or stay? Stay is better for tweaking.
            } else {
                Alert.alert("Error", "Failed to send configuration.");
            }
        } catch (e) {
            Alert.alert("Error", e.message);
        }
    };

    return (
        <View style={styles.container}>
            <View style={styles.header}>
                <TouchableOpacity onPress={onBack}>
                    <Text style={styles.backLink}>← Back</Text>
                </TouchableOpacity>
                <Text style={styles.headerTitle}>Device Settings</Text>
                <View style={{ width: 50 }} />
            </View>

            <ScrollView style={styles.scroll}>
                <View style={styles.section}>
                    <Text style={styles.sectionTitle}>LED Configuration</Text>
                    
                    <View style={styles.row}>
                        <Text style={styles.label}>Number of LEDs</Text>
                        <TextInput 
                            style={styles.input} 
                            value={config.numLeds} 
                            onChangeText={t => setConfig({...config, numLeds: t})} 
                            keyboardType="numeric"
                        />
                    </View>

                    <Text style={styles.label}>Colors</Text>
                    <View style={styles.row}>
                        {config.colors.map((c, i) => (
                            <TouchableOpacity 
                                key={i}
                                style={[styles.colorPreview, { backgroundColor: `#${c.length===6?c:'ccc'}` }]} 
                                onPress={() => openColorPicker(i)}
                            >
                                <Text style={styles.colorPreviewText}>#{c}</Text>
                            </TouchableOpacity>
                        ))}
                    </View>
                </View>

                <View style={styles.section}>
                    <Text style={styles.sectionTitle}>Brightness</Text>
                    <View style={styles.row}>
                        <Text style={styles.label}>Auto-Brightness</Text>
                        <Switch 
                            value={config.autobrightness} 
                            onValueChange={v => setConfig({...config, autobrightness: v})} 
                        />
                    </View>
                    {!config.autobrightness && (
                        <View style={styles.row}>
                            <Text style={styles.label}>Brightness (%)</Text>
                            <TextInput 
                                style={styles.input} 
                                value={config.brightnessPct} 
                                onChangeText={t => setConfig({...config, brightnessPct: t})} 
                                keyboardType="numeric"
                            />
                        </View>
                    )}
                </View>

                <View style={styles.section}>
                    <Text style={styles.sectionTitle}>Sensors & Thresholds</Text>
                    <View style={styles.row}>
                        <Text style={styles.label}>Proximity (cm)</Text>
                        <TextInput 
                            style={styles.input} 
                            value={config.distanceThresholdCm} 
                            onChangeText={t => setConfig({...config, distanceThresholdCm: t})} 
                            keyboardType="numeric"
                        />
                    </View>

                    <Text style={styles.label}>Humidity Thresholds (%)</Text>
                    <View style={styles.row}>
                        {config.humidityThresholds.map((h, i) => (
                            <TextInput 
                                key={i}
                                style={[styles.input, { flex: 1, marginRight: 5 }]} 
                                value={h} 
                                onChangeText={t => updateThres(i, t)}
                                keyboardType="numeric"
                            />
                        ))}
                    </View>
                </View>

                <TouchableOpacity style={styles.saveBtn} onPress={saveConfig}>
                    <Text style={styles.saveBtnText}>Save Configuration</Text>
                </TouchableOpacity>
                <View style={{height: 40}}/>
            </ScrollView>

            {/* Color Picker Modal */}
            <Modal visible={pickerVisible} transparent animationType="slide">
                 <View style={styles.modalOverlay}>
                     <View style={styles.modalContent}>
                         <Text style={styles.modalTitle}>Select Color</Text>
                         <View style={styles.colorGrid}>
                             {PRESET_COLORS.map(color => (
                                 <TouchableOpacity 
                                     key={color} 
                                     style={[styles.paramColor, {backgroundColor: `#${color}`}]}
                                     onPress={() => selectColor(color)}
                                 />
                             ))}
                         </View>
                         {/* Manual Input Fallback */}
                         <Text style={[styles.label, {marginTop:15}]}>Or Type Hex:</Text>
                         <TextInput 
                            style={styles.input} 
                            placeholder="RRGGBB" 
                            value={config.colors[activeColorIndex] || ''}
                            onChangeText={t => updateColorManual(activeColorIndex, t)}
                            maxLength={6}
                        />
                         <TouchableOpacity style={styles.closeBtn} onPress={() => setPickerVisible(false)}>
                             <Text style={{color:'#007AFF', fontSize:16}}>Close</Text>
                         </TouchableOpacity>
                     </View>
                 </View>
            </Modal>
        </View>
    );
}

const styles = StyleSheet.create({
    container: { flex: 1, backgroundColor: '#f2f2f7', paddingTop: 50 },
    header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingHorizontal: 20, marginBottom: 20 },
    backLink: { color: '#007AFF', fontSize: 18 },
    headerTitle: { fontSize: 20, fontWeight: 'bold' },
    scroll: { flex: 1, paddingHorizontal: 20 },
    section: { backgroundColor: '#fff', borderRadius: 12, padding: 15, marginBottom: 20, shadowColor: '#000', shadowOpacity: 0.05, shadowRadius: 5, elevation: 2 },
    sectionTitle: { fontSize: 16, fontWeight: '700', marginBottom: 15, color: '#333' },
    row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 15 },
    label: { fontSize: 16, color: '#666' },
    input: { borderWidth: 1, borderColor: '#e5e5ea', borderRadius: 8, padding: 10, minWidth: 60, textAlign: 'center', fontSize: 16, backgroundColor: '#f9f9f9' },
    saveBtn: { backgroundColor: '#007AFF', padding: 16, borderRadius: 12, alignItems: 'center' },
    saveBtnText: { color: '#fff', fontSize: 18, fontWeight: 'bold' },

    // New Styles
    colorPreview: { flex: 1, height: 50, marginRight: 8, borderRadius: 8, justifyContent: 'center', alignItems: 'center', borderWidth:1, borderColor:'#ccc' },
    colorPreviewText: { color: '#fff', fontWeight: 'bold', textShadowColor: '#000', textShadowRadius: 2 },
    
    modalOverlay: { flex:1, backgroundColor:'rgba(0,0,0,0.5)', justifyContent: 'flex-end' },
    modalContent: { backgroundColor: '#fff', borderTopLeftRadius: 20, borderTopRightRadius: 20, padding: 20, maxHeight: 600 },
    modalTitle: { fontSize: 18, fontWeight: 'bold', marginBottom: 20, textAlign: 'center' },
    colorGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'center', gap: 15 },
    paramColor: { width: 45, height: 45, borderRadius: 22.5,  borderWidth:1, borderColor:'#eee'},
    closeBtn: { marginTop: 20, alignItems: 'center', padding: 10 }
});
