const sessionManager = require('../sessions/sessionManager');

function setupSocketIO(io) {
  io.on('connection', (socket) => {
    console.log(`[Socket] Client connected: ${socket.id}`);

    // Frontend starts a new session
    socket.on('session:start', (data) => {
      const { deviceId, exercise } = data;
      console.log(`[Socket] Session start: ${deviceId} - ${exercise}`);
      const session = sessionManager.startSession(deviceId, exercise);
      // Let everyone know (or just the room)
      io.emit('session:updated', { status: 'started', session });
    });

    // Frontend ends the session
    socket.on('session:end', async (data) => {
      const { deviceId } = data;
      console.log(`[Socket] Session end: ${deviceId}`);
      const summary = await sessionManager.endSession(deviceId);
      if (summary) {
        io.emit('session:summary', summary);
      } else {
        socket.emit('error', { message: 'Failed to end/save session.' });
      }
    });

    // Frontend resets the current set (throws away buffered reps)
    socket.on('session:reset', (data) => {
      const { deviceId } = data;
      console.log(`[Socket] Session reset: ${deviceId}`);
      if (sessionManager.resetCurrentSet(deviceId)) {
        io.emit('session:reset', { deviceId });
      }
    });

    // Frontend changes exercise mode
    socket.on('exercise:change', (data) => {
      const { deviceId, exercise } = data;
      console.log(`[Socket] Exercise changed to: ${exercise}`);
      io.emit('exercise:changed', { deviceId, exercise });
    });

    socket.on('disconnect', () => {
      console.log(`[Socket] Client disconnected: ${socket.id}`);
    });
  });
}

module.exports = setupSocketIO;
