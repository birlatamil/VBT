import { create } from 'zustand';

export const useSessionStore = create((set) => ({
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
  
  setConnectionStatus: (status) => set({ connectionStatus: status }),
  setExercise: (exercise) => set({ exercise }),
  setSessionStatus: (status) => set({ sessionStatus: status }),
  setLiveTelemetry: (velocity, state) => set({ liveVelocity: velocity, liveState: state }),
  
  addRep: (rep, stats) => set((state) => ({ 
    reps: [...state.reps, rep],
    projectedRpe: stats.projected_rpe,
    velocityLossPct: stats.velocity_loss_pct
  })),
  
  resetSet: () => set({ reps: [], projectedRpe: 0, velocityLossPct: 0, insights: [] }),
  
  endSession: (summary) => set({ 
    sessionStatus: 'idle', 
    reps: [],
    projectedRpe: 0,
    velocityLossPct: 0,
    insights: summary.insights
  }),
}));
