import React from 'react';
import { motion } from 'framer-motion';
import { useSessionStore } from '../store/sessionStore';

export function RepCounter() {
  const { reps } = useSessionStore();
  const count = reps.length;

  return (
    <div className="glass-card p-6 flex flex-col items-center justify-center">
      <h2 className="text-gray-400 text-sm font-semibold uppercase tracking-wider mb-2">Reps</h2>
      <motion.div
        key={count}
        initial={{ scale: 0.5, y: -20, opacity: 0 }}
        animate={{ scale: 1, y: 0, opacity: 1 }}
        className="text-6xl font-black text-white drop-shadow-lg"
      >
        {count}
      </motion.div>
    </div>
  );
}
