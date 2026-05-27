import { io } from 'socket.io-client';

const BACKEND_URL = import.meta.env.VITE_BACKEND_URL || 'http://localhost:3001';

// Create a single instance of the socket connection
export const socket = io(BACKEND_URL, {
  autoConnect: false, // Wait until we explicitly connect
  transports: ['websocket'],
});
