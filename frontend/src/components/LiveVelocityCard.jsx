import React from 'react';
import { motion } from 'framer-motion';
import { useSessionStore } from '../store/sessionStore';
import { getVelocityZone } from '../utils/velocityZones';
import { formatVelocity } from '../utils/formatters';

export function LiveVelocityCard() {
  const { liveVelocity } = useSessionStore();
  const zone = getVelocityZone(liveVelocity);

  return (
    <div className="glass-card p-6 flex flex-col items-center justify-center relative overflow-hidden h-full min-h-[250px]">
      <div className="absolute inset-0 opacity-20 pointer-events-none">
        <motion.div
          className={`w-full h-full ${zone.bg} blur-3xl`}
          animate={{ scale: [1, 1.2, 1], opacity: [0.5, 0.8, 0.5] }}
          transition={{ repeat: Infinity, duration: 2 }}
        />
      </div>
      
      <h2 className="text-gray-400 text-sm font-semibold uppercase tracking-wider mb-2 z-10">Live Velocity (m/s)</h2>
      
      <motion.div 
        key={liveVelocity}
        initial={{ scale: 0.8, opacity: 0 }}
        animate={{ scale: 1, opacity: 1 }}
        className="z-10"
      >
        <span className={`text-7xl font-bold tracking-tighter drop-shadow-2xl ${zone.color}`}>
          {formatVelocity(liveVelocity)}
        </span>
      </motion.div>
      
      <div className={`mt-4 px-4 py-1 rounded-full text-sm font-medium border border-white/10 ${zone.bg} ${zone.color} z-10`}>
        {zone.label}
      </div>
    </div>
  );
}
