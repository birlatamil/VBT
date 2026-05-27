import { create } from 'zustand';

export const useSessionStore = create((set, get) => ({
  deviceId: 'esp32_001',
  exercise: 'squat',
  sessionStatus: 'idle', // 'idle' | 'started'
  reps: [],
  liveVelocity: 0,
  liveState: 0,
  projectedRpe: 0,
  velocityLossPct: 0,
  insights: [],
  connectionStatus: 'disconnected', // 'connected' | 'disconnected'
  espConnectionStatus: 'disconnected', // 'connected' | 'disconnected'
  espTimeout: null,
  
  setConnectionStatus: (status) => set({ connectionStatus: status }),
  setExercise: (exercise) => set({ exercise }),
  setSessionStatus: (status) => set({ sessionStatus: status }),
  
  setLiveTelemetry: (velocity, state) => {
    const { espTimeout } = get();
    if (espTimeout) clearTimeout(espTimeout);
    
    const newTimeout = setTimeout(() => {
      set({ espConnectionStatus: 'disconnected', liveVelocity: 0 });
    }, 3000);

    set({ 
      liveVelocity: velocity, 
      liveState: state,
      espConnectionStatus: 'connected',
      espTimeout: newTimeout
    });
  },
  
  addRep: (rep, stats) => {
    const { espTimeout, reps } = get();
    if (espTimeout) clearTimeout(espTimeout);
    
    const newTimeout = setTimeout(() => {
      set({ espConnectionStatus: 'disconnected', liveVelocity: 0 });
    }, 3000);

    set({ 
      reps: [...reps, rep],
      projectedRpe: stats.projected_rpe,
      velocityLossPct: stats.velocity_loss_pct,
      espConnectionStatus: 'connected',
      espTimeout: newTimeout
    });
  },
  
  resetSet: () => set({ reps: [], projectedRpe: 0, velocityLossPct: 0, insights: [] }),
  
  endSession: (summary) => set({ 
    sessionStatus: 'idle', 
    reps: [],
    projectedRpe: 0,
    velocityLossPct: 0,
    insights: summary.insights
  }),
}));
