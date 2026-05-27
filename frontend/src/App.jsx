import React, { useState } from 'react';
import { Dashboard } from './pages/Dashboard';
import { History } from './pages/History';

function App() {
  const [currentTab, setCurrentTab] = useState('dashboard');

  return (
    <div className="relative min-h-screen bg-background">
      {/* Navigation Layer */}
      <div className="absolute top-0 left-0 right-0 z-50 p-4 pointer-events-none">
        <div className="max-w-xs mx-auto glass-card flex p-1 pointer-events-auto shadow-2xl">
          <button 
            onClick={() => setCurrentTab('dashboard')}
            className={`flex-1 py-1.5 text-sm font-medium rounded-lg transition-all ${currentTab === 'dashboard' ? 'bg-white/10 text-white' : 'text-gray-400 hover:text-gray-200'}`}
          >
            Dashboard
          </button>
          <button 
            onClick={() => setCurrentTab('history')}
            className={`flex-1 py-1.5 text-sm font-medium rounded-lg transition-all ${currentTab === 'history' ? 'bg-white/10 text-white' : 'text-gray-400 hover:text-gray-200'}`}
          >
            History
          </button>
        </div>
      </div>
      
      {/* Route Content with top padding for navbar */}
      <div className="pt-12 min-h-screen">
        {currentTab === 'dashboard' ? <Dashboard /> : <History />}
      </div>
    </div>
  );
}

export default App;
