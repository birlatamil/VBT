const express = require('express');
const router = express.Router();
const sessionManager = require('../sessions/sessionManager');

// Injected io instance from server.js
module.exports = (io) => {
  
  // POST /api/ingest
  // Receives data from ESP32
  router.post('/', (req, res) => {
    const data = req.body;

    if (!data.device_id) {
      return res.status(400).json({ error: 'Missing device_id' });
    }

    if (data.type === 'heartbeat') {
      // Just a live stream update (velocity, state)
      io.emit('heartbeat', data);
      return res.status(200).json({ status: 'ok' });
    }

    // Otherwise, it's a completed rep
    const session = sessionManager.getActiveSession(data.device_id);
    if (session) {
      const savedRep = sessionManager.addRep(data.device_id, data);
      
      // Calculate real-time analytics for UI update
      const { calculateVelocityLoss } = require('../services/velocityAnalytics');
      const { calculateRPE } = require('../analytics/rpeEngine');
      
      const vLoss = calculateVelocityLoss(session.reps);
      const avgVel = session.reps.reduce((s, r) => s + r.avg_velocity, 0) / session.reps.length;
      const projRpe = calculateRPE(vLoss, session.reps.length, avgVel);

      // Broadcast to frontend
      io.emit('rep:live', {
        device_id: data.device_id,
        rep: { ...savedRep, profile: data.profile || [] },
        session_stats: {
          total_reps: session.reps.length,
          velocity_loss_pct: vLoss,
          projected_rpe: projRpe
        }
      });
    } else {
      // ESP32 sent a rep but session isn't started yet on the frontend.
      // We could auto-start a session or just drop/log it. Let's auto-start for robustness.
      console.log(`[Ingest] Received rep but no active session for ${data.device_id}. Auto-starting session.`);
      const newSession = sessionManager.startSession(data.device_id, data.exercise || 'squat');
      const savedRep = sessionManager.addRep(data.device_id, data);
      io.emit('session:updated', { status: 'started', session: newSession });
      io.emit('rep:live', {
        device_id: data.device_id,
        rep: { ...savedRep, profile: data.profile || [] },
        session_stats: { total_reps: 1, velocity_loss_pct: 0, projected_rpe: 6 }
      });
    }

    res.status(200).json({ status: 'ok' });
  });

  return router;
};
