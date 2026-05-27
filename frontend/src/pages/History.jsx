import React, { useEffect, useState } from 'react';

const BACKEND_URL = import.meta.env.VITE_BACKEND_URL || 'http://localhost:3001';

export function History() {
  const [sessions, setSessions] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetch(`${BACKEND_URL}/api/history`)
      .then(res => res.json())
      .then(data => {
        setSessions(data);
        setLoading(false);
      })
      .catch(err => {
        console.error("Failed to load history", err);
        setLoading(false);
      });
  }, []);

  return (
    <div className="min-h-screen p-6 space-y-6">
      <header className="flex justify-between items-center glass-card p-4">
        <h1 className="text-2xl font-bold text-gradient">Session History</h1>
      </header>

      <main className="glass-card p-6 min-h-[500px]">
        {loading ? (
          <div className="text-gray-400">Loading history...</div>
        ) : sessions.length === 0 ? (
          <div className="text-gray-500">No sessions recorded yet.</div>
        ) : (
          <div className="space-y-4">
            {sessions.map(session => (
              <div key={session.id} className="p-4 rounded-xl border border-border bg-surface/50">
                <div className="flex justify-between items-center mb-2">
                  <h3 className="font-semibold text-lg capitalize">{session.exercise}</h3>
                  <span className="text-sm text-gray-400">
                    {new Date(session.started_at).toLocaleString()}
                  </span>
                </div>
                <div className="grid grid-cols-4 gap-4 text-sm mt-4">
                  <div>
                    <span className="text-gray-500">Reps:</span> 
                    <span className="font-medium ml-2">{session.reps?.length || 0}</span>
                  </div>
                  <div>
                    <span className="text-gray-500">Projected RPE:</span> 
                    <span className="font-medium text-amber-500 ml-2">{session.projected_rpe?.toFixed(1) || '-'}</span>
                  </div>
                  <div>
                    <span className="text-gray-500">Vel. Loss:</span> 
                    <span className="font-medium text-red-500 ml-2">{session.velocity_loss_pct?.toFixed(1) || '-'}%</span>
                  </div>
                </div>
              </div>
            ))}
          </div>
        )}
      </main>
    </div>
  );
}
