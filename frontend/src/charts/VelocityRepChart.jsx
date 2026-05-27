import React from 'react';
import { BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, Cell } from 'recharts';
import { useSessionStore } from '../store/sessionStore';
import { getVelocityZone } from '../utils/velocityZones';

export function VelocityRepChart() {
  const { reps } = useSessionStore();
  
  if (reps.length === 0) {
    return (
      <div className="h-full flex items-center justify-center text-gray-500">
        No reps yet in this set
      </div>
    );
  }

  return (
    <ResponsiveContainer width="100%" height="100%">
      <BarChart data={reps} margin={{ top: 20, right: 30, left: 0, bottom: 5 }}>
        <CartesianGrid strokeDasharray="3 3" stroke="#374151" vertical={false} />
        <XAxis dataKey="rep_number" stroke="#9CA3AF" tick={{fill: '#9CA3AF'}} tickLine={false} axisLine={false} />
        <YAxis stroke="#9CA3AF" tick={{fill: '#9CA3AF'}} tickLine={false} axisLine={false} />
        <Tooltip 
          cursor={{fill: 'rgba(255,255,255,0.05)'}}
          contentStyle={{ backgroundColor: '#111827', borderColor: '#374151', borderRadius: '8px' }}
          itemStyle={{ color: '#F3F4F6' }}
          formatter={(value) => [`${value.toFixed(2)} m/s`, 'Avg Velocity']}
          labelFormatter={(label) => `Rep ${label}`}
        />
        <Bar dataKey="avg_velocity" radius={[4, 4, 0, 0]}>
          {reps.map((entry, index) => {
            const zone = getVelocityZone(entry.avg_velocity);
            // Convert tailwind text class to hex (approx)
            let color = '#3B82F6';
            if (zone.color.includes('red')) color = '#EF4444';
            if (zone.color.includes('amber')) color = '#F59E0B';
            if (zone.color.includes('green')) color = '#22C55E';
            if (zone.color.includes('purple')) color = '#A855F7';
            return <Cell key={`cell-${index}`} fill={color} />;
          })}
        </Bar>
      </BarChart>
    </ResponsiveContainer>
  );
}
