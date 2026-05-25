#ifndef WEBPAGE_H
#define WEBPAGE_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Barbell VBT PRO</title>
    <style>
        :root {
            --bg-base: #020617;
            --bg-card: #0f172a;
            --bg-card-hover: #1e293b;
            --accent: #38bdf8;
            --concentric: #a3e635;
            --eccentric: #f87171;
            --text-main: #f8fafc;
            --text-muted: #64748b;
        }
        * { box-sizing: border-box; font-family: 'Inter', system-ui, -apple-system, sans-serif; }
        body { background-color: var(--bg-base); color: var(--text-main); margin: 0; padding: 20px; text-align: center; overscroll-behavior: none; }
        
        /* Typography */
        h1 { margin: 0 0 20px 0; font-weight: 900; color: var(--text-main); font-size: 1.8rem; letter-spacing: 2px; text-transform: uppercase; }
        .label { font-size: 0.75rem; font-weight: 700; color: var(--text-muted); text-transform: uppercase; letter-spacing: 1.5px; }
        
        /* Layout */
        .container { max-width: 500px; margin: 0 auto; }
        .card { background: var(--bg-card); border-radius: 20px; padding: 20px; margin-bottom: 20px; box-shadow: 0 10px 30px -10px rgba(0,0,0,0.8); border: 1px solid #1e293b; }
        .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 20px; }
        
        /* Lift Mode Selector */
        .segmented-control { display: flex; background: var(--bg-base); border-radius: 12px; padding: 4px; border: 1px solid #1e293b; margin-bottom: 20px; }
        .seg-btn { flex: 1; padding: 12px 0; text-align: center; color: var(--text-muted); font-weight: 700; font-size: 0.9rem; cursor: pointer; border-radius: 10px; transition: all 0.3s; }
        .seg-btn.active { background: var(--bg-card-hover); color: var(--accent); box-shadow: 0 2px 10px rgba(0,0,0,0.5); }
        
        /* Live Velocity */
        .live-vel { font-size: 5rem; font-weight: 900; line-height: 1; margin: 10px 0; text-shadow: 0 0 20px rgba(56, 189, 248, 0.2); transition: color 0.1s; }
        .live-unit { font-size: 1.2rem; color: var(--text-muted); font-weight: 600; }
        
        /* Stats */
        .stat-val { font-size: 2rem; font-weight: 800; margin-top: 5px; }
        
        /* State Badge */
        .state-badge { display: inline-block; padding: 6px 16px; border-radius: 30px; background: #334155; font-size: 0.85rem; font-weight: 800; letter-spacing: 1px; transition: all 0.2s; margin-bottom: 10px; }
        .state-active { background: #eab308; color: #000; box-shadow: 0 0 15px rgba(234, 179, 8, 0.4); }
        
        /* Chart Container */
        .chart-container { position: relative; height: 220px; width: 100%; border-radius: 12px; background: var(--bg-base); border: 1px solid #1e293b; overflow: hidden; margin-top: 10px;}
        canvas { display: block; width: 100%; height: 100%; }
        .chart-labels { display: flex; justify-content: space-between; font-size: 0.7rem; color: var(--text-muted); padding: 5px 10px 0 10px; }
        
        /* Buttons */
        .btn-primary { background: linear-gradient(135deg, #0ea5e9, #2563eb); color: white; border: none; padding: 16px; font-size: 1.1rem; font-weight: 800; border-radius: 16px; cursor: pointer; width: 100%; box-shadow: 0 10px 20px -5px rgba(37, 99, 235, 0.5); transition: transform 0.2s; }
        .btn-primary:active { transform: translateY(2px); }
        .btn-danger { background: linear-gradient(135deg, #ef4444, #b91c1c); box-shadow: 0 10px 20px -5px rgba(239, 68, 68, 0.5); }
        
        /* Modal */
        .modal-overlay { position: fixed; top: 0; left: 0; right: 0; bottom: 0; background: rgba(0,0,0,0.8); backdrop-filter: blur(5px); display: none; align-items: center; justify-content: center; z-index: 100; }
        .modal { background: var(--bg-card); padding: 30px; border-radius: 24px; width: 90%; max-width: 350px; border: 1px solid #1e293b; }
        .rpe-grid { display: grid; grid-template-columns: repeat(5, 1fr); gap: 10px; margin: 20px 0; }
        .rpe-btn { background: var(--bg-base); border: 1px solid #334155; color: white; padding: 15px 0; border-radius: 12px; font-weight: 800; font-size: 1.1rem; cursor: pointer; transition: all 0.2s; }
        .rpe-btn:active, .rpe-btn.selected { background: var(--accent); border-color: var(--accent); color: #000; }
        
        /* History */
        .history-list { text-align: left; margin-top: 20px; }
        .history-item { background: var(--bg-card); padding: 15px; border-radius: 16px; margin-bottom: 10px; border-left: 4px solid var(--accent); display: flex; justify-content: space-between; align-items: center; }
        .hist-title { font-weight: 800; font-size: 1.1rem; }
        .hist-meta { font-size: 0.8rem; color: var(--text-muted); margin-top: 4px; }
        .hist-rpe { background: #334155; padding: 4px 10px; border-radius: 8px; font-weight: 800; font-size: 0.9rem; }
    </style>
</head>
<body>
    <div class="container">
        <h1>VBT PRO</h1>
        
        <div class="segmented-control">
            <div class="seg-btn active" onclick="setMode(0, this)">SQUAT</div>
            <div class="seg-btn" onclick="setMode(1, this)">BENCH</div>
            <div class="seg-btn" onclick="setMode(2, this)">DEADLIFT</div>
        </div>

        <div class="card">
            <div class="state-badge" id="stateBadge">IDLE</div>
            <div class="label">Velocity</div>
            <div class="live-vel" id="vel">0.00</div>
            <div class="live-unit">m/s</div>
        </div>

        <div class="grid-2">
            <div class="card" style="margin:0;">
                <div class="label">Reps in Set</div>
                <div class="stat-val" id="reps" style="color:var(--text-main);">0</div>
            </div>
            <div class="card" style="margin:0;">
                <div class="label">Last Peak</div>
                <div class="stat-val" id="peak" style="color:var(--concentric);">0.00</div>
            </div>
        </div>

        <div class="card">
            <div class="label" style="text-align:left;">Rep Diagnostic Graph</div>
            <div class="chart-container">
                <canvas id="repChart"></canvas>
            </div>
            <div class="chart-labels">
                <span>Start</span>
                <span>Velocity vs Position</span>
                <span>End</span>
            </div>
        </div>

        <button class="btn-primary btn-danger" onclick="endSet()">END SET & LOG RPE</button>
        
        <div class="history-list" id="historyList">
            <div class="label" style="margin-bottom: 10px;">Set History</div>
            <!-- History items injected here -->
        </div>
    </div>

    <!-- RPE Modal -->
    <div class="modal-overlay" id="rpeModal">
        <div class="modal">
            <h2 style="margin-top:0;">Log Set</h2>
            <p style="color:var(--text-muted); font-size:0.9rem;">How hard was that set?</p>
            <div class="rpe-grid">
                <button class="rpe-btn" onclick="selectRPE(6, this)">6</button>
                <button class="rpe-btn" onclick="selectRPE(7, this)">7</button>
                <button class="rpe-btn" onclick="selectRPE(8, this)">8</button>
                <button class="rpe-btn" onclick="selectRPE(9, this)">9</button>
                <button class="rpe-btn" onclick="selectRPE(10, this)">10</button>
            </div>
            <button class="btn-primary" onclick="saveSet()">SAVE SET</button>
        </div>
    </div>

    <script>
        // --- State ---
        let currentMode = 0; // 0:Squat, 1:Bench, 2:Deadlift
        let currentReps = 0;
        let lastPeak = 0;
        let currentRPE = 8;
        let selectedRpeBtn = null;
        let setHistory = JSON.parse(localStorage.getItem('vbt_history') || '[]');
        
        const modeNames = ["Squat", "Bench", "Deadlift"];
        const states = ["IDLE", "DESCENDING", "BOTTOM PAUSE", "ASCENDING", "LOCKOUT"];

        // --- Canvas Graph ---
        const canvas = document.getElementById('repChart');
        const ctx = canvas.getContext('2d');
        
        // Handle retina displays
        function resizeCanvas() {
            const rect = canvas.parentElement.getBoundingClientRect();
            canvas.width = rect.width * window.devicePixelRatio;
            canvas.height = rect.height * window.devicePixelRatio;
            ctx.scale(window.devicePixelRatio, window.devicePixelRatio);
            drawEmptyGraph();
        }
        window.addEventListener('resize', resizeCanvas);
        
        function drawEmptyGraph() {
            const w = canvas.width / window.devicePixelRatio;
            const h = canvas.height / window.devicePixelRatio;
            ctx.clearRect(0, 0, w, h);
            
            // Draw center line (0 m/s)
            ctx.beginPath();
            ctx.moveTo(0, h/2);
            ctx.lineTo(w, h/2);
            ctx.strokeStyle = '#1e293b';
            ctx.lineWidth = 2;
            ctx.stroke();
            
            ctx.fillStyle = '#64748b';
            ctx.font = '10px Inter';
            ctx.fillText('0 m/s', 5, h/2 - 5);
        }

        function drawGraph(profileData) {
            const w = canvas.width / window.devicePixelRatio;
            const h = canvas.height / window.devicePixelRatio;
            
            drawEmptyGraph();
            if(!profileData || profileData.length === 0) return;

            // X is position, Y is velocity.
            // Velocity bounds: -3.0 to 3.0 m/s
            const MAX_V = 3.0; 
            
            // Find position bounds to scale X axis
            let minX = profileData[0][0];
            let maxX = profileData[0][0];
            profileData.forEach(p => {
                if(p[0] < minX) minX = p[0];
                if(p[0] > maxX) maxX = p[0];
            });
            const rangeX = (maxX - minX) || 1.0;
            const paddingX = rangeX * 0.1;
            
            ctx.beginPath();
            ctx.strokeStyle = '#38bdf8';
            ctx.lineWidth = 3;
            ctx.lineJoin = 'round';
            
            profileData.forEach((p, i) => {
                const pos = p[0];
                const vel = p[1];
                
                // Map X: minX-padding to maxX+padding
                const normX = (pos - (minX - paddingX)) / (rangeX + paddingX*2);
                const screenX = normX * w;
                
                // Map Y: -3.0 to 3.0
                const normY = (vel + MAX_V) / (MAX_V * 2); // 0.0 to 1.0
                const screenY = h - (normY * h); // Invert Y
                
                if(i === 0) ctx.moveTo(screenX, screenY);
                else ctx.lineTo(screenX, screenY);
            });
            
            ctx.stroke();
            
            // Fill gradient below curve
            const gradient = ctx.createLinearGradient(0, 0, 0, h);
            gradient.addColorStop(0, 'rgba(56, 189, 248, 0.4)');
            gradient.addColorStop(1, 'rgba(56, 189, 248, 0.0)');
            
            ctx.lineTo(w, h/2);
            ctx.lineTo(0, h/2);
            ctx.fillStyle = gradient;
            ctx.fill();
        }

        // --- Logic ---
        function setMode(modeId, btnEl) {
            currentMode = modeId;
            document.querySelectorAll('.seg-btn').forEach(b => b.classList.remove('active'));
            btnEl.classList.add('active');
            
            fetch('/set_mode?mode=' + modeId, { method: 'POST' }).catch(e=>console.log(e));
        }

        let lastRepCount = 0;

        // Establish Real-Time Server-Sent Events (SSE) stream
        const eventSource = new EventSource('/events');

        eventSource.onmessage = function(event) {
            const data = JSON.parse(event.data);
            const velEl = document.getElementById('vel');
            velEl.innerText = data.v.toFixed(2);
            
            // Color code live velocity
            if (data.v > 0.05) velEl.style.color = 'var(--concentric)';
            else if (data.v < -0.05) velEl.style.color = 'var(--eccentric)';
            else velEl.style.color = 'var(--text-main)';

            // Badge status
            const badge = document.getElementById('stateBadge');
            badge.innerText = states[data.s] || "UNKNOWN";
            if (data.s === 1 || data.s === 3) badge.classList.add('state-active');
            else badge.classList.remove('state-active');

            // Rep logic within set
            if (data.r > lastRepCount) {
                currentReps++;
                document.getElementById('reps').innerText = currentReps;
                lastPeak = data.p;
                document.getElementById('peak').innerText = lastPeak.toFixed(2);
                lastRepCount = data.r;
                
                // Fetch graph profile when rep finishes
                fetchProfile();
            }
        };

        eventSource.onerror = function(e) {
            console.error("SSE connection lost. EventSource reconnecting automatically...");
            document.getElementById('stateBadge').innerText = "DISCONNECTED";
            document.getElementById('stateBadge').classList.remove('state-active');
        };

        function fetchProfile() {
            fetch('/rep_profile')
                .then(r => r.json())
                .then(profileArray => {
                    drawGraph(profileArray);
                })
                .catch(e => console.error(e));
        }

        // --- Set / RPE Handling ---
        function endSet() {
            if (currentReps === 0) return alert("No reps recorded yet!");
            document.getElementById('rpeModal').style.display = 'flex';
        }

        function selectRPE(val, btn) {
            currentRPE = val;
            if (selectedRpeBtn) selectedRpeBtn.classList.remove('selected');
            btn.classList.add('selected');
            selectedRpeBtn = btn;
        }

        function saveSet() {
            const setObj = {
                id: Date.now(),
                mode: modeNames[currentMode],
                reps: currentReps,
                peak: lastPeak,
                rpe: currentRPE,
                date: new Date().toLocaleDateString()
            };
            setHistory.unshift(setObj);
            localStorage.setItem('vbt_history', JSON.stringify(setHistory));
            
            document.getElementById('rpeModal').style.display = 'none';
            currentReps = 0;
            document.getElementById('reps').innerText = "0";
            drawEmptyGraph();
            
            renderHistory();
        }

        function renderHistory() {
            const list = document.getElementById('historyList');
            list.innerHTML = '<div class="label" style="margin-bottom: 10px;">Set History</div>';
            
            setHistory.slice(0, 5).forEach(s => {
                const item = document.createElement('div');
                item.className = 'history-item';
                item.innerHTML = `
                    <div>
                        <div class="hist-title">${s.mode} <span style="font-weight:400; color:var(--text-muted);">x</span> ${s.reps}</div>
                        <div class="hist-meta">Peak: ${s.peak.toFixed(2)}m/s • ${s.date}</div>
                    </div>
                    <div class="hist-rpe">RPE ${s.rpe}</div>
                `;
                list.appendChild(item);
            });
        }

        // Initial setup
        setTimeout(resizeCanvas, 100);
        renderHistory();
    </script>
</body>
</html>
)rawliteral";

#endif
