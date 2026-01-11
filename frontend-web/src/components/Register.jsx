import React, { useState } from 'react';
import { api } from '../api/client';
import { useAuth } from '../api/auth';
import { Link, useNavigate } from 'react-router-dom';

export default function Register() {
  const [formData, setFormData] = useState({
      firstName: '', lastName: '', email: '', password: ''
  });
  const [error, setError] = useState('');
  const { login } = useAuth();
  const navigate = useNavigate();

  const handleSubmit = async (e) => {
    e.preventDefault();
    try {
      const data = await api.auth.register(formData);
      login(data.token);
      navigate('/');
    } catch (err) {
      setError(err.message);
    }
  };

  return (
    <div className="max-w-xs mx-auto mt-10">
      <h2 className="text-2xl font-bold mb-4">Register</h2>
      {error && <div className="text-red-500 mb-2">{error}</div>}
      <form onSubmit={handleSubmit} className="space-y-4">
        <div>
           <input type="text" placeholder="First Name" required className="w-full p-2 border rounded" 
             value={formData.firstName} onChange={e => setFormData({...formData, firstName:e.target.value})} />
        </div>
        <div>
           <input type="text" placeholder="Last Name" required className="w-full p-2 border rounded" 
             value={formData.lastName} onChange={e => setFormData({...formData, lastName:e.target.value})} />
        </div>
        <div>
           <input type="email" placeholder="Email" required className="w-full p-2 border rounded" 
             value={formData.email} onChange={e => setFormData({...formData, email:e.target.value})} />
        </div>
        <div>
           <input type="password" placeholder="Password" required className="w-full p-2 border rounded" 
             value={formData.password} onChange={e => setFormData({...formData, password:e.target.value})} />
        </div>
        <button type="submit" className="w-full bg-green-600 text-white p-2 rounded hover:bg-green-700">Register</button>
      </form>
      <div className="mt-4 text-center">
        <Link to="/login" className="text-blue-500">Already have an account?</Link>
      </div>
    </div>
  );
}
