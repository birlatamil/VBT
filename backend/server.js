const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const cors = require('cors');
require('dotenv').config();

const setupSocketIO = require('./websocket/socketHandler');

const app = express();
const server = http.createServer(app);

// Enable CORS
app.use(cors({
  origin: '*', // Allow all origins for dev
  methods: ['GET', 'POST']
}));

app.use(express.json());

// Setup Socket.IO
const io = new Server(server, {
  cors: {
    origin: '*',
    methods: ['GET', 'POST']
  },
  transports: ['websocket']
});

setupSocketIO(io);

// Routes
const ingestRoute = require('./routes/ingest')(io);
const historyRoute = require('./routes/history');

app.use('/api/ingest', ingestRoute);
app.use('/api/history', historyRoute);

// Basic health check
app.get('/health', (req, res) => {
  res.status(200).json({ status: 'ok', time: new Date().toISOString() });
});

const PORT = process.env.PORT || 3001;

server.listen(PORT, () => {
  console.log(`[Server] VBT Backend running on port ${PORT}`);
});
