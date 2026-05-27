const { createClient } = require('@supabase/supabase-js');
require('dotenv').config();

const supabaseUrl = process.env.SUPABASE_URL || 'http://localhost:54321';
const supabaseKey = process.env.SUPABASE_KEY || 'public-anon-key';

const supabase = createClient(supabaseUrl, supabaseKey);

module.exports = supabase;
