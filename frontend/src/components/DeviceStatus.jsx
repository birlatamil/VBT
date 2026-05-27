import React from 'react';
import { Activity, Wifi, WifiOff } from 'lucide-react';
import { useSessionStore } from '../store/sessionStore';

export function DeviceStatus() {
  const { connectionStatus } = useSessionStore();
  const isConnected = connectionStatus === 'connected';

  return (
    <div className="flex items-center space-x-2 bg-surface backdrop-blur-md px-4 py-2 rounded-full border border-border">
      {isConnected ? (
        <Wifi className="w-4 h-4 text-green-400" />
      ) : (
        <WifiOff className="w-4 h-4 text-red-400" />
      )}
      <span className="text-sm font-medium text-gray-200">
        {isConnected ? 'ESP32 Connected' : 'Disconnected'}
      </span>
      {isConnected && (
        <span className="relative flex h-2 w-2 ml-2">
          <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-green-400 opacity-75"></span>
          <span className="relative inline-flex rounded-full h-2 w-2 bg-green-500"></span>
        </span>
      )}
    </div>
  );
}
