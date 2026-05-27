import React from 'react';
import { Wifi, WifiOff, Server, HardDrive } from 'lucide-react';
import { useSessionStore } from '../store/sessionStore';

export function DeviceStatus() {
  const { connectionStatus, espConnectionStatus } = useSessionStore();
  const isServerConnected = connectionStatus === 'connected';
  const isEspConnected = espConnectionStatus === 'connected';

  return (
    <div className="flex items-center space-x-4 bg-surface backdrop-blur-md px-4 py-2 rounded-full border border-border">
      {/* Server Status */}
      <div className="flex items-center space-x-2" title="Server Connection">
        {isServerConnected ? (
          <Server className="w-4 h-4 text-green-400" />
        ) : (
          <Server className="w-4 h-4 text-red-400" />
        )}
        <span className="text-sm font-medium text-gray-200">
          {isServerConnected ? 'Server' : 'Server Offline'}
        </span>
      </div>

      <div className="w-px h-4 bg-border"></div>

      {/* ESP32 Status */}
      <div className="flex items-center space-x-2" title="ESP32 Connection">
        {isEspConnected ? (
          <HardDrive className="w-4 h-4 text-green-400" />
        ) : (
          <HardDrive className="w-4 h-4 text-gray-500" />
        )}
        <span className={`text-sm font-medium ${isEspConnected ? 'text-gray-200' : 'text-gray-500'}`}>
          {isEspConnected ? 'ESP32 Active' : 'ESP32 Idle'}
        </span>
        {isEspConnected && (
          <span className="relative flex h-2 w-2 ml-1">
            <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-green-400 opacity-75"></span>
            <span className="relative inline-flex rounded-full h-2 w-2 bg-green-500"></span>
          </span>
        )}
      </div>
    </div>
  );
}
