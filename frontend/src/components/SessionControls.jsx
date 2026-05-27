import React from 'react';
import { Play, Square, RotateCcw } from 'lucide-react';
import { socket } from '../services/socket';
import { useSessionStore } from '../store/sessionStore';

export function SessionControls() {
  const { sessionStatus, deviceId, exercise } = useSessionStore();
  const isStarted = sessionStatus === 'started';

  const handleStart = () => {
    socket.emit('session:start', { deviceId, exercise });
  };

  const handleEnd = () => {
    socket.emit('session:end', { deviceId });
  };

  const handleReset = () => {
    socket.emit('session:reset', { deviceId });
  };

  return (
    <div className="flex items-center space-x-4">
      {!isStarted ? (
        <button 
          onClick={handleStart}
          className="flex items-center px-6 py-3 bg-primary-600 hover:bg-primary-500 text-white font-semibold rounded-xl transition-all shadow-lg shadow-primary-500/20 active:scale-95"
        >
          <Play className="w-5 h-5 mr-2" />
          Start Set
        </button>
      ) : (
        <>
          <button 
            onClick={handleEnd}
            className="flex items-center px-6 py-3 bg-red-600 hover:bg-red-500 text-white font-semibold rounded-xl transition-all shadow-lg shadow-red-500/20 active:scale-95"
          >
            <Square className="w-5 h-5 mr-2" />
            End Set
          </button>
          <button 
            onClick={handleReset}
            className="flex items-center px-4 py-3 bg-surface hover:bg-white/10 text-gray-300 font-semibold rounded-xl transition-all border border-border active:scale-95"
            title="Reset Current Set (Throw away reps)"
          >
            <RotateCcw className="w-5 h-5" />
          </button>
        </>
      )}
    </div>
  );
}
