import React, { useEffect, useState, useRef } from 'react';
import { useParams } from 'react-router-dom';
import { api } from '../api/client';

export default function DeviceControl() {
  const { id } = useParams();
  const [device, setDevice] = useState(null);
  const [telemetry, setTelemetry] = useState(null);
  const [loading, setLoading] = useState(true);

  // Configuration State
  const [config, setConfig] = useState({
      colors: ["FF0000", "00FF00", "0000FF"],
      humidityThresholds: [30.0, 50.0, 70.0, 90.0]
  });

  useEffect(() => {
    loadData();
    const interval = setInterval(loadData, 5000); // Poll telemetry
    return () => clearInterval(interval);
  }, [id]);

  const loadData = async () => {
    try {
        const d = await api.devices.get(id);
        const t = await api.telemetry.getLatest(id);
        setDevice(d);
        if (t) setTelemetry(t);
        setLoading(false);
    } catch (e) {
        console.error(e);
        setLoading(false);
    }
  };

  const sendConfig = async () => {
    try {
        if (!device) return;
        await api.devices.control(id, 'SET_CONFIG', {
            colors: config.colors,
            humidityThresholds: config.humidityThresholds
        });
        alert('Configuration sent!');
    } catch(e) { alert('Failed to send config'); }
  };

  if(!device) return <div>Loading...</div>;

  return (
    <div className="space-y-6">
        <h1 className="text-2xl font-bold">{device.name}</h1>
        
        {/* Telemetry Card */}
        <div className="bg-white p-4 rounded shadow space-y-4">
            <h3 className="font-bold text-gray-600 border-b pb-2">Sensor Readings</h3>
            <div className="grid grid-cols-2 gap-4">
                <div className="p-3 bg-blue-50 rounded">
                    <div className="text-sm text-gray-500">Temperature</div>
                    <div className="text-xl font-bold">{telemetry?.temperature?.toFixed(1) || '--'} °C</div>
                </div>
                <div className="p-3 bg-green-50 rounded">
                    <div className="text-sm text-gray-500">Humidity</div>
                    <div className="text-xl font-bold">{telemetry?.humidity?.toFixed(1) || '--'} %</div>
                </div>
                <div className="p-3 bg-yellow-50 rounded">
                    <div className="text-sm text-gray-500">Lux</div>
                    <div className="text-xl font-bold">{telemetry?.brightness?.toFixed(0) || '--'}</div>
                </div>
                <div className="p-3 bg-purple-50 rounded">
                    <div className="text-sm text-gray-500">Motion</div>
                    <div className="text-xl font-bold">{telemetry?.motionDetected ? 'YES' : 'NO'}</div>
                </div>
            </div>
            <div className="text-xs text-gray-400 text-right">
                Last Update: {telemetry ? new Date(telemetry.timestamp).toLocaleTimeString() : 'Never'}
            </div>
        </div>

        {/* Configuration Card */}
        <div className="bg-white p-4 rounded shadow space-y-4">
            <h3 className="font-bold text-gray-600 border-b pb-2">Configuration</h3>
            
            <div>
                <label className="text-sm font-bold block mb-2">LED Colors (Hex)</label>
                <div className="flex gap-2">
                    {config.colors.map((c, i) => (
                        <input 
                            key={i} 
                            type="color" 
                            className="h-10 w-full"
                            value={`#${c}`}
                            onChange={(e) => {
                                const newColors = [...config.colors];
                                newColors[i] = e.target.value.replace('#', '').toUpperCase();
                                setConfig({...config, colors: newColors});
                            }}
                        />
                    ))}
                </div>
            </div>

            <div>
                <label className="text-sm font-bold block mb-2">Humidity Thresholds (%)</label>
                <div className="grid grid-cols-4 gap-2">
                    {config.humidityThresholds.map((t, i) => (
                        <input 
                            key={i} 
                            type="number" 
                            className="border p-1 rounded w-full text-center"
                            value={t}
                            onChange={(e) => {
                                const newTh = [...config.humidityThresholds];
                                newTh[i] = parseFloat(e.target.value);
                                setConfig({...config, humidityThresholds: newTh});
                            }}
                        />
                    ))}
                </div>
            </div>

            <button onClick={sendConfig} className="w-full bg-blue-600 text-white p-3 rounded font-bold">
                Update Settings
            </button>
        </div>
    </div>
  );
}
