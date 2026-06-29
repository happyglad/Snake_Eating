import argparse
import json
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

try:
    import msvcrt
except ImportError:
    msvcrt = None

try:
    import serial
except ImportError:
    serial = None


ARROW_MAP = {
    b"H": b"U",
    b"P": b"D",
    b"K": b"L",
    b"M": b"R",
}


class SnakeStats:
    def __init__(self):
        self.lock = threading.Lock()
        self.current = {
            "mode": "normal",
            "score": 0,
            "high": 0,
            "length": 0,
            "duration": 0,
            "state": "idle",
            "reason": "",
            "last_event": "",
            "updated_at": time.time(),
        }
        self.sessions = []
        self.events = []

    def snapshot(self):
        with self.lock:
            sessions = list(self.sessions)
            ranked = sorted(sessions, key=lambda row: row["score"], reverse=True)[:8]
            avg_score = sum(row["score"] for row in sessions) / len(sessions) if sessions else 0
            avg_duration = sum(row["duration"] for row in sessions) / len(sessions) if sessions else 0
            return {
                "current": dict(self.current),
                "sessions": sessions[-20:],
                "rankings": ranked,
                "summary": {
                    "games": len(sessions),
                    "avg_score": round(avg_score, 1),
                    "avg_duration": round(avg_duration, 1),
                    "best_score": ranked[0]["score"] if ranked else 0,
                },
                "events": list(self.events[-30:]),
            }

    def apply_event(self, event_name, fields, raw):
        now = time.time()
        with self.lock:
            self.current["last_event"] = raw
            self.current["updated_at"] = now

            if event_name == "START":
                self.current.update({
                    "mode": fields.get("mode", self.current["mode"]),
                    "score": 0,
                    "length": 4,
                    "duration": 0,
                    "state": "playing",
                    "reason": "",
                })
            elif event_name == "FOOD":
                self.current["score"] = int_field(fields, "score", self.current["score"])
                self.current["length"] = int_field(fields, "length", self.current["length"])
                self.current["duration"] = int_field(fields, "duration", self.current["duration"])
                self.current["state"] = "playing"
            elif event_name == "END":
                score = int_field(fields, "score", self.current["score"])
                high = int_field(fields, "high", self.current["high"])
                duration = int_field(fields, "duration", self.current["duration"])
                reason = fields.get("reason", "end")
                self.current.update({
                    "score": score,
                    "high": high,
                    "duration": duration,
                    "state": "game over",
                    "reason": reason,
                })
                self.sessions.append({
                    "index": len(self.sessions) + 1,
                    "mode": self.current["mode"],
                    "score": score,
                    "high": high,
                    "duration": duration,
                    "reason": reason,
                    "ended_at": time.strftime("%H:%M:%S"),
                })

            self.events.append({
                "time": time.strftime("%H:%M:%S"),
                "name": event_name,
                "raw": raw,
            })


def int_field(fields, key, default):
    try:
        return int(fields.get(key, default))
    except (TypeError, ValueError):
        return default


def parse_snake_line(line):
    if not line.startswith("SNAKE,"):
        return None
    parts = [part.strip() for part in line.split(",")]
    if len(parts) < 2:
        return None
    fields = {}
    for part in parts[2:]:
        if "=" in part:
            key, value = part.split("=", 1)
            fields[key.strip()] = value.strip()
    return parts[1].upper(), fields


INDEX_HTML = """<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Snake Dashboard</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #101418;
      --panel: #182026;
      --panel-2: #202a31;
      --text: #eef4f1;
      --muted: #9aaba6;
      --green: #54d36a;
      --cyan: #5fc6de;
      --yellow: #f3c64e;
      --red: #ef6a5b;
      --line: #314048;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "Segoe UI", Arial, sans-serif;
      background: var(--bg);
      color: var(--text);
    }
    header {
      height: 72px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 0 28px;
      background: #141b20;
      border-bottom: 1px solid var(--line);
    }
    h1 { margin: 0; font-size: 24px; font-weight: 700; }
    .status { color: var(--muted); font-size: 14px; }
    main { padding: 24px; max-width: 1180px; margin: 0 auto; }
    .grid {
      display: grid;
      grid-template-columns: repeat(4, minmax(0, 1fr));
      gap: 14px;
    }
    .card {
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 8px;
      padding: 16px;
      min-height: 104px;
    }
    .label { color: var(--muted); font-size: 13px; text-transform: uppercase; }
    .value { margin-top: 10px; font-size: 34px; font-weight: 800; }
    .green { color: var(--green); }
    .cyan { color: var(--cyan); }
    .yellow { color: var(--yellow); }
    .red { color: var(--red); }
    .wide {
      margin-top: 16px;
      display: grid;
      grid-template-columns: 1.1fr 0.9fr;
      gap: 16px;
    }
    table {
      width: 100%;
      border-collapse: collapse;
      margin-top: 10px;
      font-size: 14px;
    }
    th, td {
      padding: 9px 8px;
      border-bottom: 1px solid var(--line);
      text-align: left;
    }
    th { color: var(--muted); font-weight: 600; }
    .bars { margin-top: 12px; display: grid; gap: 9px; }
    .bar-row { display: grid; grid-template-columns: 42px 1fr 46px; gap: 10px; align-items: center; }
    .bar-track { height: 16px; background: var(--panel-2); border-radius: 4px; overflow: hidden; }
    .bar-fill { height: 100%; background: var(--cyan); min-width: 2px; }
    .events { color: var(--muted); font-family: Consolas, monospace; font-size: 13px; line-height: 1.6; max-height: 280px; overflow: auto; }
    @media (max-width: 820px) {
      .grid, .wide { grid-template-columns: 1fr; }
      header { padding: 0 18px; }
      main { padding: 16px; }
    }
  </style>
</head>
<body>
  <header>
    <h1>Snake Dashboard</h1>
    <div class="status" id="status">Waiting for serial events</div>
  </header>
  <main>
    <section class="grid">
      <div class="card"><div class="label">Current Score</div><div class="value yellow" id="score">0</div></div>
      <div class="card"><div class="label">High Score</div><div class="value cyan" id="high">0</div></div>
      <div class="card"><div class="label">Duration</div><div class="value green"><span id="duration">0</span>s</div></div>
      <div class="card"><div class="label">Mode / State</div><div class="value" id="mode">normal</div></div>
    </section>
    <section class="wide">
      <div class="card">
        <div class="label">Highest Score Ranking</div>
        <table>
          <thead><tr><th>#</th><th>Mode</th><th>Score</th><th>Time</th><th>Reason</th></tr></thead>
          <tbody id="ranking"></tbody>
        </table>
      </div>
      <div class="card">
        <div class="label">Game Duration Statistics</div>
        <div class="bars" id="bars"></div>
      </div>
    </section>
    <section class="wide">
      <div class="card">
        <div class="label">Summary</div>
        <table>
          <tbody>
            <tr><th>Games</th><td id="games">0</td></tr>
            <tr><th>Average Score</th><td id="avgScore">0</td></tr>
            <tr><th>Average Duration</th><td id="avgDuration">0s</td></tr>
            <tr><th>Last Reason</th><td id="reason"></td></tr>
          </tbody>
        </table>
      </div>
      <div class="card">
        <div class="label">Serial Events</div>
        <div class="events" id="events"></div>
      </div>
    </section>
  </main>
  <script>
    async function load() {
      const res = await fetch('/api/state');
      const data = await res.json();
      const c = data.current;
      document.getElementById('score').textContent = c.score;
      document.getElementById('high').textContent = c.high;
      document.getElementById('duration').textContent = c.duration;
      document.getElementById('mode').textContent = c.mode + ' / ' + c.state;
      document.getElementById('reason').textContent = c.reason || '';
      document.getElementById('games').textContent = data.summary.games;
      document.getElementById('avgScore').textContent = data.summary.avg_score;
      document.getElementById('avgDuration').textContent = data.summary.avg_duration + 's';
      document.getElementById('status').textContent = c.last_event || 'Waiting for serial events';

      document.getElementById('ranking').innerHTML = data.rankings.map((row, i) =>
        `<tr><td>${i + 1}</td><td>${row.mode}</td><td>${row.score}</td><td>${row.duration}s</td><td>${row.reason}</td></tr>`
      ).join('') || '<tr><td colspan="5">No finished games yet</td></tr>';

      const maxDuration = Math.max(1, ...data.sessions.map(row => row.duration));
      document.getElementById('bars').innerHTML = data.sessions.slice(-10).map(row => {
        const width = Math.max(3, Math.round(row.duration * 100 / maxDuration));
        return `<div class="bar-row"><span>#${row.index}</span><div class="bar-track"><div class="bar-fill" style="width:${width}%"></div></div><span>${row.duration}s</span></div>`;
      }).join('') || '<div class="status">No duration data yet</div>';

      document.getElementById('events').innerHTML = data.events.slice().reverse().map(row =>
        `<div>${row.time} ${row.raw}</div>`
      ).join('');
    }
    load();
    setInterval(load, 700);
  </script>
</body>
</html>
"""


def make_handler(stats):
    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            parsed = urlparse(self.path)
            if parsed.path == "/api/state":
                body = json.dumps(stats.snapshot()).encode("utf-8")
                self.send_response(200)
                self.send_header("Content-Type", "application/json; charset=utf-8")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
                return

            body = INDEX_HTML.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def log_message(self, fmt, *args):
            return

    return Handler


def serial_worker(ser, stats, stop_event):
    buffer = ""
    while not stop_event.is_set():
        data = ser.read(128)
        if not data:
            time.sleep(0.02)
            continue
        text = data.decode("ascii", errors="ignore")
        buffer += text
        while "\n" in buffer:
            line, buffer = buffer.split("\n", 1)
            line = line.strip()
            parsed = parse_snake_line(line)
            if parsed:
                event_name, fields = parsed
                stats.apply_event(event_name, fields, line)
                print(line)


def keyboard_worker(ser, stop_event):
    if msvcrt is None:
        return
    print("Controls: arrows move, Enter starts/restarts, B/M switches mode, Esc quits.")
    while not stop_event.is_set():
        if not msvcrt.kbhit():
            time.sleep(0.01)
            continue
        ch = msvcrt.getch()
        if ch in (b"\x00", b"\xe0"):
            code = msvcrt.getch()
            cmd = ARROW_MAP.get(code)
            if cmd:
                ser.write(cmd)
                ser.flush()
        elif ch == b"\r":
            ser.write(b"S")
            ser.flush()
        elif ch in (b"b", b"B", b"m", b"M"):
            ser.write(b"B")
            ser.flush()
        elif ch == b"\x1b":
            stop_event.set()


def parse_args():
    parser = argparse.ArgumentParser(description="Snake serial dashboard with keyboard control.")
    parser.add_argument("port", help="Serial port, for example COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--http-port", type=int, default=8080)
    return parser.parse_args()


def main():
    if serial is None:
        print("pyserial is required. Install it with: pip install pyserial")
        return 1

    args = parse_args()
    stats = SnakeStats()
    stop_event = threading.Event()

    with serial.Serial(args.port, args.baud, timeout=0) as ser:
        try:
            ser.dtr = False
            ser.rts = False
        except IOError:
            pass

        server = ThreadingHTTPServer((args.host, args.http_port), make_handler(stats))
        server.timeout = 0.5
        serial_thread = threading.Thread(target=serial_worker, args=(ser, stats, stop_event), daemon=True)
        keyboard_thread = threading.Thread(target=keyboard_worker, args=(ser, stop_event), daemon=True)
        serial_thread.start()
        keyboard_thread.start()

        print(f"Dashboard: http://{args.host}:{args.http_port}")
        try:
            while not stop_event.is_set():
                server.handle_request()
        except KeyboardInterrupt:
            stop_event.set()
        finally:
            server.server_close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
