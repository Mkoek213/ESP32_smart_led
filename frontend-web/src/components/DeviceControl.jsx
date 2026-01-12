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

    const [error, setError] = useState(null);

    const loadData = async () => {
        try {
            const d = await api.devices.get(id);
            const t = await api.telemetry.getLatest(id);
            setDevice(d);
            if (t) setTelemetry(t);
            setLoading(false);
        } catch (e) {
            console.error(e);
            setError(e.message);
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
        } catch (e) { alert('Failed to send config'); }
    };

    if (loading) return <div>Loading...</div>;

    if (!device) return (
        <div className="flex flex-col items-center justify-center h-64 space-y-4">
            <h2 className="text-xl font-bold text-gray-700">Device Not Found</h2>
            <p className="text-gray-500">This device may have been deleted or does not exist.</p>
            {error && <p className="text-red-500 bg-red-100 p-2 rounded">Debug Error: {error}</p>}
            <a href="/" className="text-blue-600 hover:underline">Return to Dashboard</a>
        </div>
    );

    return (
        <div className="space-y-6">
            <div className="flex justify-between items-center">
                <h1 className="text-2xl font-bold">{device.name}</h1>
                <a href="/" className="text-sm text-blue-600 hover:underline">Back to Dashboard</a>
            </div>

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
                        <div className="text-sm text-gray-500">Pressure</div>
                        <div className="text-xl font-bold">{telemetry?.pressure?.toFixed(1) || '--'} hPa</div>
                    </div>
                    <div className="p-3 bg-purple-50 rounded">
                        <div className="text-sm text-gray-500">Motion</div>
                        <div className="text-xl font-bold">{telemetry?.personCount > 0 ? 'YES' : 'NO'}</div>
                    </div>
                </div>
                <div className="text-xs text-gray-400 text-right">
                    Last Update: {telemetry?.timestamp ? new Date(telemetry.timestamp).toLocaleTimeString() : 'Never'}
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
                                    setConfig({ ...config, colors: newColors });
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
                                    setConfig({ ...config, humidityThresholds: newTh });
                                }}
                            />
                        ))}
                    </div>
                </div>

                <button onClick={sendConfig} className="w-full bg-blue-600 text-white p-3 rounded font-bold">
                    Update Settings
                </button>
            </div>

            {/* Firmware Update Card */}
            <div className="bg-white p-4 rounded shadow space-y-4">
                <h3 className="font-bold text-gray-600 border-b pb-2">Firmware Update</h3>
                <FirmwareUploader />
            </div>
        </div>
    );
}

function FirmwareUploader() {
    const [file, setFile] = useState(null);
    const [version, setVersion] = useState('');
    const [uploading, setUploading] = useState(false);
    const [message, setMessage] = useState('');

    const handleUpload = async () => {
        if (!file || !version) {
            setMessage('Please select a file and enter a version.');
            return;
        }

        setUploading(true);
        setMessage('');

        try {
            await api.firmware.upload(file, version);
            setMessage('Upload successful! ESP32 will update shortly.');
            setFile(null);
            setVersion('');
        } catch (e) {
            setMessage('Upload failed: ' + e.message);
        } finally {
            setUploading(false);
        }
    };

    return (
        <div className="space-y-3">
            <div>
                <label className="block text-sm font-medium text-gray-700">Firmware File (.bin)</label>
                <input
                    type="file"
                    accept=".bin"
                    className="mt-1 block w-full text-sm text-gray-500
                        file:mr-4 file:py-2 file:px-4
                        file:rounded-full file:border-0
                        file:text-sm file:font-semibold
                        file:bg-blue-50 file:text-blue-700
                        hover:file:bg-blue-100"
                    onChange={(e) => setFile(e.target.files[0])}
                />
            </div>
            <div>
                <label className="block text-sm font-medium text-gray-700">Version</label>
                <input
                    type="text"
                    placeholder="e.g. 1.0.1"
                    className="mt-1 block w-full border border-gray-300 rounded-md shadow-sm p-2"
                    value={version}
                    onChange={(e) => setVersion(e.target.value)}
                />
            </div>
            <button
                onClick={handleUpload}
                disabled={uploading}
                className={`w-full p-2 rounded text-white font-bold ${uploading ? 'bg-gray-400' : 'bg-green-600 hover:bg-green-700'}`}
            >
                {uploading ? 'Uploading...' : 'Upload & Flash'}
            </button>
            {message && (
                <div className={`text-sm p-2 rounded ${message.includes('failed') ? 'bg-red-100 text-red-700' : 'bg-green-100 text-green-700'}`}>
                    {message}
                </div>
            )}
        </div>
    );
}
