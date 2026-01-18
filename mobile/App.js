import React, { useState, useEffect } from 'react';
import { StyleSheet, View, Platform, PermissionsAndroid, Alert, Text } from 'react-native';
import { StatusBar } from 'expo-status-bar';
import { BleManager } from 'react-native-ble-plx';
import * as Location from 'expo-location';

import DashboardScreen from './components/DashboardScreen';
import ProvisioningScreen from './components/ProvisioningScreen';
import SettingsScreen from './components/SettingsScreen';

// Singleton BleManager
const manager = new BleManager();
const DEFAULT_HOST = 'http://192.168.0.186:8080';

export default function App() {
  const [currentScreen, setCurrentScreen] = useState('dashboard'); // 'dashboard' | 'provisioning' | 'settings'
  const [permissionGranted, setPermissionGranted] = useState(false);
  const [host, setHost] = useState(DEFAULT_HOST);
  const [settingsDeviceId, setSettingsDeviceId] = useState(null);

  useEffect(() => {
    requestPermissions();
  }, []);

  const requestPermissions = async () => {
    if (Platform.OS === 'android') {
      // Request Expo Location Permission (this handles Manifest)
      let { status } = await Location.requestForegroundPermissionsAsync();
      if (status !== 'granted') {
        Alert.alert('Permission denied', 'Location permission is required for Bluetooth scanning.');
        return;
      }

      // Request Android 12+ Bluetooth Permissions
      if (Platform.Version >= 31) {
        const granted = await PermissionsAndroid.requestMultiple([
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION
        ]);

        const allGranted = Object.values(granted).every(
          perm => perm === PermissionsAndroid.RESULTS.GRANTED
        );

        setPermissionGranted(true);
      } else {
        setPermissionGranted(true);
      }
    } else {
      // iOS permissions are handled by plist and triggered on usage
      setPermissionGranted(true);
    }
  };

  const renderScreen = () => {
    if (currentScreen === 'dashboard') {
      return (
        <DashboardScreen
          host={host}
          setHost={setHost}
          onNavigateProvision={() => setCurrentScreen('provisioning')}
          onNavigateSettings={(deviceId) => {
            setSettingsDeviceId(deviceId);
            setCurrentScreen('settings');
          }}
        />
      );
    } else if (currentScreen === 'provisioning') {
      return (
        <ProvisioningScreen
          manager={manager}
          host={host}
          onFinish={() => setCurrentScreen('dashboard')}
          onCancel={() => setCurrentScreen('dashboard')}
        />
      );
    } else if (currentScreen === 'settings') {
      return (
        <SettingsScreen
          host={host}
          deviceId={settingsDeviceId}
          onBack={() => setCurrentScreen('dashboard')}
        />
      );
    }
  };

  return (
    <View style={styles.container}>
      {renderScreen()}
      <StatusBar style="auto" />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#fff',
  },
});
