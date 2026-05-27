const express = require('express');
const router = express.Router();
const supabase = require('../database/supabase');

// GET /api/history
// Returns all past sessions with their reps and insights
router.get('/', async (req, res) => {
  try {
    const { data: sessions, error } = await supabase
      .from('sessions')
      .select(`
        *,
        reps (*),
        insights (*)
      `)
      .order('started_at', { ascending: false });

    if (error) throw error;

    res.json(sessions);
  } catch (err) {
    console.error('Error fetching history:', err);
    res.status(500).json({ error: 'Failed to fetch history' });
  }
});

module.exports = router;
