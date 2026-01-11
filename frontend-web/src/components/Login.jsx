import React, { useState } from 'react';
import { api } from '../api/client';
import { useAuth } from '../api/auth';
import { Link, useNavigate } from 'react-router-dom';

export default function Login() {
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const { login } = useAuth();
  const navigate = useNavigate();

  const handleSubmit = async (e) => {
    e.preventDefault();
    try {
      const data = await api.auth.login({ email, password });
      login(data.token);
      navigate('/');
    } catch (err) {
      setError('Invalid credentials');
    }
  };

  return (
    <div className="max-w-xs mx-auto mt-10">
      <h2 className="text-2xl font-bold mb-4">Login</h2>
      {error && <div className="text-red-500 mb-2">{error}</div>}
      <form onSubmit={handleSubmit} className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-700">Email</label>
          <input 
            type="email" 
            required
            className="mt-1 block w-full rounded border-gray-300 shadow-sm p-2 border" 
            value={email}
            onChange={e => setEmail(e.target.value)}
          />
        </div>
        <div>
          <label className="block text-sm font-medium text-gray-700">Password</label>
          <input 
            type="password" 
            required
            className="mt-1 block w-full rounded border-gray-300 shadow-sm p-2 border" 
            value={password}
            onChange={e => setPassword(e.target.value)}
          />
        </div>
        <button type="submit" className="w-full bg-blue-600 text-white p-2 rounded hover:bg-blue-700">Sign In</button>
      </form>
      <div className="mt-4 text-center">
        <Link to="/register" className="text-blue-500">Create an account</Link>
      </div>
    </div>
  );
}
