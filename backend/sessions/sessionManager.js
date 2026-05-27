const supabase = require('../database/supabase');
const { calculateRPE } = require('../analytics/rpeEngine');
const { generateInsights } = require('../analytics/insightsEngine');
const { calculateVelocityLoss } = require('../services/velocityAnalytics');

/**
 * In-memory state of active sessions per device.
 * Key: device_id
 * Value: { id, exercise, started_at, reps: [] }
 */
const activeSessions = {};

const sessionManager = {
  
  startSession: (deviceId, exercise) => {
    activeSessions[deviceId] = {
      id: crypto.randomUUID(), // Assuming Node 19+ or will use uuid fallback if needed. Let's use simple Math.random for now to avoid extra deps if crypto not available globally, but crypto.randomUUID() is standard.
      device_id: deviceId,
      exercise: exercise,
      started_at: new Date().toISOString(),
      reps: []
    };
    return activeSessions[deviceId];
  },

  getActiveSession: (deviceId) => {
    return activeSessions[deviceId];
  },

  addRep: (deviceId, repData) => {
    const session = activeSessions[deviceId];
    if (session) {
      session.reps.push({
        rep_number: session.reps.length + 1,
        ...repData
      });
      return session.reps[session.reps.length - 1];
    }
    return null;
  },

  resetCurrentSet: (deviceId) => {
    if (activeSessions[deviceId]) {
      activeSessions[deviceId].reps = [];
      return true;
    }
    return false;
  },

  endSession: async (deviceId) => {
    const session = activeSessions[deviceId];
    if (!session) return null;

    const endedAt = new Date().toISOString();
    const velocityLossPct = calculateVelocityLoss(session.reps);
    const avgVel = session.reps.length > 0 ? session.reps.reduce((s, r) => s + r.avg_velocity, 0) / session.reps.length : 0;
    const projectedRpe = calculateRPE(velocityLossPct, session.reps.length, avgVel);
    const insights = generateInsights(session.reps, velocityLossPct, projectedRpe);

    // Prepare data for DB
    const sessionRecord = {
      // let supabase generate UUID
      device_id: session.device_id,
      exercise: session.exercise,
      started_at: session.started_at,
      ended_at: endedAt,
      projected_rpe: projectedRpe,
      velocity_loss_pct: velocityLossPct,
    };

    try {
      // 1. Insert Session
      const { data: insertedSession, error: sessionErr } = await supabase
        .from('sessions')
        .insert([sessionRecord])
        .select()
        .single();
      
      if (sessionErr) throw sessionErr;

      const dbSessionId = insertedSession.id;

      // 2. Insert Reps
      if (session.reps.length > 0) {
        const repsRecords = session.reps.map(r => ({
          session_id: dbSessionId,
          rep_number: r.rep_number,
          avg_velocity: r.avg_velocity,
          peak_velocity: r.peak_velocity,
          ecc_time: r.ecc_time,
          pause_time: r.pause_time,
          con_time: r.con_time,
          total_time: r.total_time,
          timestamp: r.timestamp
        }));

        const { error: repsErr } = await supabase
          .from('reps')
          .insert(repsRecords);

        if (repsErr) throw repsErr;
      }

      // 3. Insert Insights
      if (insights.length > 0) {
        const insightsRecords = insights.map(i => ({
          session_id: dbSessionId,
          type: i.type,
          message: i.message
        }));

        const { error: insightsErr } = await supabase
          .from('insights')
          .insert(insightsRecords);
          
        if (insightsErr) throw insightsErr;
      }

      // Clear session from memory
      delete activeSessions[deviceId];

      return {
        session: insertedSession,
        reps: session.reps,
        insights,
        velocityLossPct,
        projectedRpe
      };

    } catch (err) {
      console.error('Failed to save session to DB:', err);
      // Don't delete session from memory, maybe try again?
      return null;
    }
  }
};

module.exports = sessionManager;
