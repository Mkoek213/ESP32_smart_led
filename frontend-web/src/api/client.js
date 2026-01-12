const BASE_URL = '/api'; // Using proxy in vite.config.js

const getHeaders = () => {
    const token = localStorage.getItem('token');
    const headers = {
        'Content-Type': 'application/json',
    };
    if (token) {
        headers['Authorization'] = `Bearer ${token}`;
    }
    return headers;
};

export const api = {
    auth: {
        register: async (data) => {
            const res = await fetch(`${BASE_URL}/register`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(data),
            });
            if (!res.ok) {
                const err = await res.text();
                throw new Error(err || 'Registration failed');
            }
            return res.json();
        },
        login: async (data) => {
            const res = await fetch(`${BASE_URL}/login`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(data),
            });
            if (!res.ok) {
                const err = await res.text();
                throw new Error(err || 'Login failed');
            }
            const result = await res.json();
            if (result.token) {
                localStorage.setItem('token', result.token);
            }
            return result;
        },
        logout: () => {
            localStorage.removeItem('token');
        }
    },
    locations: {
        list: async () => {
            const res = await fetch(`${BASE_URL}/locations`, {
                headers: getHeaders()
            });
            if (!res.ok) throw new Error('Failed to fetch locations');
            return res.json();
        },
        create: async (data) => {
            const res = await fetch(`${BASE_URL}/locations`, {
                method: 'POST',
                headers: getHeaders(),
                body: JSON.stringify(data)
            });
            if (!res.ok) throw new Error('Failed to create location');
            return res.json();
        },
        get: async (id) => {
            const res = await fetch(`${BASE_URL}/locations/${id}`, {
                headers: getHeaders()
            });
            if (!res.ok) throw new Error('Failed to fetch location');
            return res.json();
        },
        delete: async (id) => {
            const res = await fetch(`${BASE_URL}/locations/${id}`, {
                method: 'DELETE',
                headers: getHeaders()
            });
            if (!res.ok) throw new Error('Failed to delete location');
        }
    },
    devices: {
        list: async () => {
            const res = await fetch(`${BASE_URL}/devices`, {
                headers: getHeaders()
            });
            if (!res.ok) throw new Error('Failed to fetch devices');
            return res.json();
        },
        get: async (id) => {
            const res = await fetch(`${BASE_URL}/devices/${id}`, {
                headers: getHeaders()
            });
            if (!res.ok) throw new Error('Failed to fetch device');
            return res.json();
        },
        claim: async (data) => {
            const res = await fetch(`${BASE_URL}/devices/claim`, {
                method: 'POST',
                headers: getHeaders(),
                body: JSON.stringify(data)
            });
            if (!res.ok) {
                const msg = await res.text();
                throw new Error(msg || 'Failed to claim device');
            }
            return res.json();
        },
        toggle: async (id, state) => {
            const res = await fetch(`${BASE_URL}/devices/${id}/toggle?state=${state}`, {
                method: 'POST',
                headers: getHeaders()
            });
            if (!res.ok) throw new Error('Failed to toggle device');
            return res.json();
        },
        updateConfig: async (id, config) => {
            const res = await fetch(`${BASE_URL}/devices/${id}/config`, {
                method: 'PUT',
                headers: getHeaders(),
                body: JSON.stringify(config)
            });
            if (!res.ok) throw new Error('Failed to update config');
            return res.json();
        },
        control: async (id, type, payload) => {
            const res = await fetch(`${BASE_URL}/devices/${id}/control`, {
                method: 'POST',
                headers: getHeaders(),
                body: JSON.stringify({ type, payload })
            });
            if (!res.ok) throw new Error('Failed to send command');
        },
        remove: async (id) => {
            const res = await fetch(`${BASE_URL}/devices/${id}`, {
                method: 'DELETE',
                headers: getHeaders()
            });
            if (!res.ok) {
                const text = await res.text();
                throw new Error(`Failed to delete device: ${res.status} ${res.statusText} - ${text}`);
            }
        }
    },
    telemetry: {
        getLatest: async (deviceId) => {
            const res = await fetch(`${BASE_URL}/telemetry/${deviceId}/latest`, {
                headers: getHeaders()
            });
            if (!res.ok) return null;

            // Handle empty body (200 OK but no content)
            const text = await res.text();
            return text ? JSON.parse(text) : null;
        }
    },
    firmware: {
        upload: async (file, version) => {
            const formData = new FormData();
            formData.append('file', file);
            formData.append('version', version);

            const headers = getHeaders();
            delete headers['Content-Type']; // Let browser set boundary for FormData
            delete headers['Authorization']; // Upload is public, avoid token validation issues

            const res = await fetch(`${BASE_URL}/firmware/upload`, {
                method: 'POST',
                headers: headers,
                body: formData
            });

            if (!res.ok) {
                const text = await res.text();
                throw new Error(text || 'Firmware upload failed');
            }
            return res.text();
        }
    }
};
