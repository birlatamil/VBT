-- 1. Create Sessions Table
CREATE TABLE public.sessions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id TEXT NOT NULL,
    exercise TEXT NOT NULL,
    started_at TIMESTAMPTZ NOT NULL,
    ended_at TIMESTAMPTZ NOT NULL,
    projected_rpe FLOAT NOT NULL,
    velocity_loss_pct FLOAT NOT NULL,
    created_at TIMESTAMPTZ DEFAULT now()
);

-- 2. Create Reps Table
CREATE TABLE public.reps (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    session_id UUID REFERENCES public.sessions(id) ON DELETE CASCADE,
    rep_number INTEGER NOT NULL,
    avg_velocity FLOAT NOT NULL,
    peak_velocity FLOAT NOT NULL,
    ecc_time FLOAT NOT NULL,
    pause_time FLOAT NOT NULL,
    con_time FLOAT NOT NULL,
    total_time FLOAT NOT NULL,
    timestamp BIGINT NOT NULL,
    created_at TIMESTAMPTZ DEFAULT now()
);

-- 3. Create Insights Table
CREATE TABLE public.insights (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    session_id UUID REFERENCES public.sessions(id) ON DELETE CASCADE,
    type TEXT NOT NULL,
    message TEXT NOT NULL,
    created_at TIMESTAMPTZ DEFAULT now()
);

-- Optional: Enable Row Level Security (RLS) if you want to secure it later, 
-- but for now we can just allow anon access for your hardware.
ALTER TABLE public.sessions ENABLE ROW LEVEL SECURITY;
ALTER TABLE public.reps ENABLE ROW LEVEL SECURITY;
ALTER TABLE public.insights ENABLE ROW LEVEL SECURITY;

CREATE POLICY "Allow anonymous inserts" ON public.sessions FOR INSERT TO anon WITH CHECK (true);
CREATE POLICY "Allow anonymous selects" ON public.sessions FOR SELECT TO anon USING (true);

CREATE POLICY "Allow anonymous inserts" ON public.reps FOR INSERT TO anon WITH CHECK (true);
CREATE POLICY "Allow anonymous selects" ON public.reps FOR SELECT TO anon USING (true);

CREATE POLICY "Allow anonymous inserts" ON public.insights FOR INSERT TO anon WITH CHECK (true);
CREATE POLICY "Allow anonymous selects" ON public.insights FOR SELECT TO anon USING (true);
