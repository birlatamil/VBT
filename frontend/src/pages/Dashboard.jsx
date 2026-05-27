import React from 'react';
import { useSocket } from '../hooks/useSocket';
import { LiveVelocityCard } from '../components/LiveVelocityCard';
import { RepCounter } from '../components/RepCounter';
import { ExerciseSelector } from '../components/ExerciseSelector';
import { DeviceStatus } from '../components/DeviceStatus';
import { SessionControls } from '../components/SessionControls';
import { VelocityRepChart } from '../charts/VelocityRepChart';
import { useSessionStore } from '../store/sessionStore';
import { Activity, Zap, AlertTriangle, Info } from 'lucide-react';

export function Dashboard() {
  // Initialize socket connection
  useSocket();

  const { projectedRpe, velocityLossPct, insights } = useSessionStore();

  return (
    <div className="min-h-screen flex flex-col p-6 space-y-6">
      
      {/* Header Area */}
      <header className="flex justify-between items-center glass-card p-4">
        <div className="flex items-center space-x-3">
          <Activity className="w-8 h-8 text-primary-500" />
          <h1 className="text-2xl font-bold text-gradient">VBT PRO</h1>
        </div>
        <div className="flex items-center space-x-6">
          <ExerciseSelector />
          <DeviceStatus />
        </div>
      </header>

      {/* Main Grid Layout */}
      <main className="flex-1 grid grid-cols-12 gap-6 min-h-0">
        
        {/* Left Column (Stats & Controls) */}
        <div className="col-span-3 flex flex-col space-y-6">
          <RepCounter />
          
          <div className="glass-card p-6 flex flex-col items-center justify-center space-y-4">
            <h2 className="text-gray-400 text-sm font-semibold uppercase tracking-wider">Projected RPE</h2>
            <div className="text-5xl font-bold text-amber-500">
              {projectedRpe ? projectedRpe.toFixed(1) : '-'}
            </div>
          </div>
          
          <div className="glass-card p-6 flex flex-col items-center justify-center space-y-4">
            <h2 className="text-gray-400 text-sm font-semibold uppercase tracking-wider">Velocity Loss</h2>
            <div className="text-5xl font-bold text-red-500">
              {velocityLossPct ? `${velocityLossPct.toFixed(1)}%` : '-'}
            </div>
          </div>
          
          <div className="mt-auto">
            <SessionControls />
          </div>
        </div>
        
        {/* Center Column (Live Display & Chart) */}
        <div className="col-span-6 flex flex-col space-y-6">
          <div className="flex-1">
            <LiveVelocityCard />
          </div>
          <div className="flex-1 glass-card p-4">
            <h2 className="text-gray-400 text-sm font-semibold uppercase tracking-wider mb-4 ml-2">Velocity vs Rep</h2>
            <div className="h-[250px]">
              <VelocityRepChart />
            </div>
          </div>
        </div>
        
        {/* Right Column (Insights Panel) */}
        <div className="col-span-3 glass-card p-6 flex flex-col">
          <div className="flex items-center space-x-2 mb-6">
            <Zap className="w-5 h-5 text-yellow-400" />
            <h2 className="text-white font-semibold">Training Insights</h2>
          </div>
          
          <div className="flex-1 space-y-4 overflow-y-auto pr-2 custom-scrollbar">
            {insights.length === 0 ? (
              <div className="text-gray-500 text-sm text-center mt-10">
                Complete a set to generate insights.
              </div>
            ) : (
              insights.map((insight, idx) => (
                <div 
                  key={idx} 
                  className={`p-4 rounded-xl border ${
                    insight.type === 'warning' ? 'bg-amber-500/10 border-amber-500/20 text-amber-200' :
                    insight.type === 'danger' ? 'bg-red-500/10 border-red-500/20 text-red-200' :
                    insight.type === 'success' ? 'bg-green-500/10 border-green-500/20 text-green-200' :
                    'bg-blue-500/10 border-blue-500/20 text-blue-200'
                  }`}
                >
                  <div className="flex items-start">
                    {insight.type === 'warning' || insight.type === 'danger' ? (
                      <AlertTriangle className="w-5 h-5 mr-3 shrink-0 mt-0.5" />
                    ) : (
                      <Info className="w-5 h-5 mr-3 shrink-0 mt-0.5" />
                    )}
                    <p className="text-sm leading-relaxed">{insight.message}</p>
                  </div>
                </div>
              ))
            )}
          </div>
        </div>
        
      </main>
    </div>
  );
}
