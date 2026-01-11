import React from 'react';
import { BrowserRouter as Router, Routes, Route, Navigate, Link } from 'react-router-dom';
import { useAuth } from './api/auth';
import Login from './components/Login';
import Register from './components/Register';
import Dashboard from './components/Dashboard';
import DeviceControl from './components/DeviceControl';
import Provisioning from './components/Provisioning';

const PrivateRoute = ({ children }) => {
  const { user } = useAuth();
  if (!user) return <Navigate to="/login" />;
  return children;
};

function App() {
  const { user, logout } = useAuth();

  return (
    <Router>
      <div className="min-h-screen bg-gray-50 text-gray-900 pb-20">
        <header className="bg-white shadow px-4 py-4 flex justify-between items-center sticky top-0 z-10">
          <Link to="/" className="text-xl font-bold text-blue-600">Smart LED</Link>
          {user && (
            <div className="flex gap-4">
              <Link to="/provision" className="text-sm font-medium text-gray-600 hover:text-blue-500">Setup WiFi</Link>
              <button onClick={logout} className="text-sm font-medium text-red-500">Logout</button>
            </div>
          )}
        </header>

        <main className="p-4 max-w-md mx-auto">
          <Routes>
            <Route path="/login" element={user ? <Navigate to="/" /> : <Login />} />
            <Route path="/register" element={user ? <Navigate to="/" /> : <Register />} />
            
            <Route path="/" element={
              <PrivateRoute>
                <Dashboard />
              </PrivateRoute>
            } />
            
            <Route path="/device/:id" element={
              <PrivateRoute>
                <DeviceControl />
              </PrivateRoute>
            } />

            <Route path="/provision" element={
              <PrivateRoute>
                <Provisioning />
              </PrivateRoute>
            } />
          </Routes>
        </main>
      </div>
    </Router>
  );
}

export default App;
