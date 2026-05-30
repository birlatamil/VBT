import React from 'react';
import { 
  LineChart, 
  Line, 
  XAxis, 
  YAxis, 
  CartesianGrid, 
  Tooltip, 
  ResponsiveContainer 
} from 'recharts';
import { useSessionStore } from '../store/sessionStore';

export function VelocityTimeChart() {
  const { latestRepProfile } = useSessionStore();

  // If there's no data yet, show a placeholder
  if (!latestRepProfile || latestRepProfile.length === 0) {
    return (
      <div className="w-full h-full flex items-center justify-center text-gray-500 text-sm">
        Complete a rep to see velocity curve
      </div>
    );
  }

  // Format data for Recharts: assuming 10ms sampling rate (100Hz) from ESP32
  // X-axis will be time in seconds
  const chartData = latestRepProfile.map((point, index) => ({
    time: (index * 0.01).toFixed(2), // 10ms per sample
    velocity: point.velocity || point, // Handle both object {velocity: x} or flat array [x, y, ...]
  }));

  return (
    <ResponsiveContainer width="100%" height="100%">
      <LineChart
        data={chartData}
        margin={{ top: 5, right: 5, left: -20, bottom: 5 }}
      >
        <CartesianGrid strokeDasharray="3 3" stroke="#334155" opacity={0.5} />
        <XAxis 
          dataKey="time" 
          stroke="#94a3b8" 
          fontSize={12}
          tickFormatter={(val) => `${val}s`}
          tick={{ fill: '#94a3b8' }}
        />
        <YAxis 
          stroke="#94a3b8" 
          fontSize={12}
          tickFormatter={(val) => `${val.toFixed(1)}`}
          tick={{ fill: '#94a3b8' }}
        />
        <Tooltip
          contentStyle={{ 
            backgroundColor: '#0f172a', 
            border: '1px solid #334155',
            borderRadius: '0.5rem',
            color: '#fff'
          }}
          itemStyle={{ color: '#0ea5e9' }}
          labelFormatter={(label) => `Time: ${label}s`}
          formatter={(value) => [Number(value).toFixed(2) + ' m/s', 'Velocity']}
        />
        <Line 
          type="monotone" 
          dataKey="velocity" 
          stroke="#0ea5e9" 
          strokeWidth={3} 
          dot={false}
          activeDot={{ r: 6, fill: '#0ea5e9', stroke: '#fff', strokeWidth: 2 }}
          isAnimationActive={false} // Disable animation to show immediately when rep finishes
        />
      </LineChart>
    </ResponsiveContainer>
  );
}
