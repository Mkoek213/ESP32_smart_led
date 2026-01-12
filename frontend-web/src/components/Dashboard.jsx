import React, { useEffect, useState } from 'react';
import { api } from '../api/client';
import { Link } from 'react-router-dom';
import { Plus, Settings, Activity } from 'lucide-react';

export default function Dashboard() {
  const [devices, setDevices] = useState([]);
  const [locations, setLocations] = useState([]);
  const [showClaim, setShowClaim] = useState(false);
  const [showLoc, setShowLoc] = useState(false);

  // Claim Form State
  const [claimData, setClaimData] = useState({ macAddress: '', proofOfPossession: '', name: '', locationId: '' });
  const [newLocName, setNewLocName] = useState('');

  const refresh = async () => {
    try {
      const [devs, locs] = await Promise.all([api.devices.list(), api.locations.list()]);
      setDevices(devs);
      setLocations(locs);
      if (locs.length > 0 && !claimData.locationId) {
        setClaimData(prev => ({ ...prev, locationId: locs[0].id }));
      }
    } catch (e) { console.error(e); }
  };

  useEffect(() => {
    refresh();
  }, []);

  const handleClaim = async (e) => {
    e.preventDefault();
    try {
      await api.devices.claim(claimData);
      setShowClaim(false);
      setClaimData({ macAddress: '', proofOfPossession: '', name: '', locationId: locations[0]?.id || '' });
      refresh();
    } catch (e) { alert(e.message); }
  };

  const handleAddLoc = async (e) => {
    e.preventDefault();
    try {
      await api.locations.create({ name: newLocName });
      setNewLocName('');
      setShowLoc(false);
      refresh();
    } catch (e) { alert(e.message); }
  };

  const handleDeleteLoc = async (id) => {
    if (!window.confirm('Are you sure you want to delete this room?')) return;
    try {
      await api.locations.delete(id);
      refresh();
    } catch (e) { alert(e.message); }
  };

  const handleDeleteDevice = async (e, id) => {
    e.preventDefault(); // Prevent navigation
    if (!window.confirm('Are you sure you want to delete this device?')) return;
    try {
      await api.devices.remove(id);
      refresh();
    } catch (e) { alert(e.message); }
  };

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <h2 className="text-xl font-bold">My Devices</h2>
        <div className="flex gap-2">
          <button onClick={() => setShowLoc(!showLoc)} className="bg-gray-200 p-2 rounded text-sm">+ Room</button>
          <button onClick={() => setShowClaim(!showClaim)} className="bg-blue-600 text-white p-2 rounded text-sm">+ Device</button>
        </div>
      </div>

      {showLoc && (
        <form onSubmit={handleAddLoc} className="bg-white p-4 rounded shadow">
          <input type="text" placeholder="Room Name (e.g. Living Room)" required className="w-full border p-2 mb-2" value={newLocName} onChange={e => setNewLocName(e.target.value)} />
          <button type="submit" className="bg-green-600 text-white w-full p-2 rounded">Create Room</button>
        </form>
      )}

      {showClaim && (
        <form onSubmit={handleClaim} className="bg-white p-4 rounded shadow space-y-2">
          <h3 className="font-bold">Claim Device</h3>
          <input placeholder="MAC Address (e.g. AA:BB:CC...)" required className="w-full border p-2" value={claimData.macAddress} onChange={e => setClaimData({ ...claimData, macAddress: e.target.value })} />
          <input placeholder="Secret Code (PoP)" required className="w-full border p-2" value={claimData.proofOfPossession} onChange={e => setClaimData({ ...claimData, proofOfPossession: e.target.value })} />
          <input placeholder="Device Name" required className="w-full border p-2" value={claimData.name} onChange={e => setClaimData({ ...claimData, name: e.target.value })} />
          <select className="w-full border p-2" value={claimData.locationId} onChange={e => setClaimData({ ...claimData, locationId: e.target.value })}>
            {locations.map(l => <option key={l.id} value={l.id}>{l.name}</option>)}
          </select>
          <button type="submit" className="bg-blue-600 text-white w-full p-2 rounded">Claim</button>
        </form>
      )}

      {/* Rooms List */}
      {locations.length > 0 && (
        <div className="flex gap-2 overflow-x-auto pb-2">
          {locations.map(l => (
            <div key={l.id} className="bg-blue-100 text-blue-800 px-3 py-1 rounded-full text-sm font-medium whitespace-nowrap flex items-center gap-2">
              {l.name}
              <button
                onClick={() => handleDeleteLoc(l.id)}
                className="text-blue-600 hover:text-red-500 font-bold ml-1 px-1"
                title="Delete Room"
              >
                ×
              </button>
            </div>
          ))}
        </div>
      )}

      <div className="grid gap-4">
        {devices.map(device => (
          <Link key={device.id} to={`/device/${device.id}`} className="block bg-white p-4 rounded shadow hover:bg-gray-50 flex justify-between items-center group">
            <div>
              <div className="font-bold text-lg">{device.name}</div>
              <div className="text-sm text-gray-500">{locations.find(l => l.id === device.locationId)?.name || 'Unassigned'}</div>
            </div>
            <div className="flex items-center gap-4">
              <div className={`w-3 h-3 rounded-full ${device.status === 'ONLINE' ? 'bg-green-500' : 'bg-gray-300'}`}></div>
              <button
                onClick={(e) => handleDeleteDevice(e, device.id)}
                className="bg-gray-100 hover:bg-red-100 text-gray-400 hover:text-red-600 p-2 rounded-full transition-colors opacity-0 group-hover:opacity-100"
                title="Delete Device"
              >
                <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M3 6h18" /><path d="M19 6v14c0 1-1 2-2 2H7c-1 0-2-1-2-2V6" /><path d="M8 6V4c0-1 1-2 2-2h4c1 0 2 1 2 2v2" /></svg>
              </button>
            </div>
          </Link>
        ))}
        {devices.length === 0 && <div className="text-center text-gray-500 py-10">No devices found. Setup WiFi or Claim one.</div>}
      </div>
    </div>
  );
}
