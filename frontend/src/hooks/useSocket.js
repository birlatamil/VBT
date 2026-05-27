import { useEffect } from 'react';
import { socket } from '../services/socket';
import { useSessionStore } from '../store/sessionStore';

export function useSocket() {
  const { 
    setConnectionStatus, 
    setLiveTelemetry, 
    addRep, 
    setSessionStatus,
    endSession
  } = useSessionStore();

  useEffect(() => {
    socket.connect();

    socket.on('connect', () => {
      setConnectionStatus('connected');
    });

    socket.on('disconnect', () => {
      setConnectionStatus('disconnected');
    });

    socket.on('heartbeat', (data) => {
      setLiveTelemetry(data.velocity, data.state);
    });

    socket.on('rep:live', (data) => {
      addRep(data.rep, data.session_stats);
    });

    socket.on('session:updated', (data) => {
      if (data.status === 'started') {
        setSessionStatus('started');
      }
    });

    socket.on('session:summary', (data) => {
      endSession(data);
    });

    return () => {
      socket.off('connect');
      socket.off('disconnect');
      socket.off('heartbeat');
      socket.off('rep:live');
      socket.off('session:updated');
      socket.off('session:summary');
      socket.disconnect();
    };
  }, []);
}
