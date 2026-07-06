import argparse
import json
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

try:
    import msvcrt
except ImportError:
    msvcrt = None

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None


ARROW_MAP = {
    b"H": b"U",
    b"P": b"D",
    b"K": b"L",
    b"M": b"R",
}


class SnakeStats:
    def __init__(self, data_file):
        self.lock = threading.Lock()
        self.data_file = Path(data_file)
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
        self.load()

    def load(self):
        if not self.data_file.exists():
            return
        try:
            with self.data_file.open("r", encoding="utf-8") as fp:
                data = json.load(fp)
        except (OSError, json.JSONDecodeError) as exc:
            print(f"Warning: could not load dashboard data from {self.data_file}: {exc}")
            return

        sessions = data.get("sessions", [])
        if not isinstance(sessions, list):
            return

        loaded = []
        for i, row in enumerate(sessions, 1):
            if not isinstance(row, dict):
                continue
            loaded.append({
                "index": int_field(row, "index", i),
                "mode": str(row.get("mode", "normal")),
                "score": int_field(row, "score", 0),
                "high": int_field(row, "high", 0),
                "duration": int_field(row, "duration", 0),
                "reason": str(row.get("reason", "end")),
                "ended_at": str(row.get("ended_at", "")),
                "ended_at_date": str(row.get("ended_at_date", "")),
            })

        self.sessions = loaded
        best_score = max((row["score"] for row in self.sessions), default=0)
        self.current["high"] = best_score

    def save(self):
        self.data_file.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "version": 1,
            "saved_at": time.strftime("%Y-%m-%d %H:%M:%S"),
            "sessions": self.sessions,
        }
        tmp_file = self.data_file.with_suffix(self.data_file.suffix + ".tmp")
        with tmp_file.open("w", encoding="utf-8") as fp:
            json.dump(payload, fp, ensure_ascii=False, indent=2)
        tmp_file.replace(self.data_file)

    def snapshot(self):
        with self.lock:
            sessions = list(self.sessions)
            ranked = sorted(
                sessions,
                key=lambda row: (row["score"], row["duration"], row["index"]),
                reverse=True,
            )[:20]
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
                    "ended_at_date": time.strftime("%Y-%m-%d"),
                })
                try:
                    self.save()
                except OSError as exc:
                    self.events.append({
                        "time": time.strftime("%H:%M:%S"),
                        "name": "SAVE_ERROR",
                        "raw": f"Could not save dashboard data: {exc}",
                    })

            self.events.append({
                "time": time.strftime("%H:%M:%S"),
                "name": event_name,
                "raw": raw,
            })

    def add_serial_line(self, raw):
        with self.lock:
            self.current["last_event"] = raw
            self.current["updated_at"] = time.time()
            self.events.append({
                "time": time.strftime("%H:%M:%S"),
                "name": "SERIAL",
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
  <title>贪吃蛇数据面板</title>
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
    .hint { margin-top: 6px; color: var(--muted); font-size: 13px; line-height: 1.5; }
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
    <h1>贪吃蛇数据面板</h1>
    <div class="status" id="status">等待串口事件</div>
  </header>
  <main>
    <section class="grid">
      <div class="card"><div class="label">当前分数</div><div class="value yellow" id="score">0</div></div>
      <div class="card"><div class="label">历史最高分</div><div class="value cyan" id="high">0</div></div>
      <div class="card"><div class="label">本局时长</div><div class="value green"><span id="duration">0</span>秒</div></div>
      <div class="card"><div class="label">模式 / 状态</div><div class="value" id="mode">普通 / 空闲</div></div>
    </section>
    <section class="wide">
      <div class="card">
        <div class="label">最佳 20 次成绩</div>
        <table>
          <thead><tr><th>排名</th><th>模式</th><th>分数</th><th>时长</th><th>结束时间</th><th>原因</th></tr></thead>
          <tbody id="ranking"></tbody>
        </table>
      </div>
      <div class="card">
        <div class="label">游戏时长统计</div>
        <div class="hint">显示最近 10 局已结束游戏的时长，单位为秒；横条长度按这 10 局中最长时长等比例显示。</div>
        <div class="bars" id="bars"></div>
      </div>
    </section>
    <section class="wide">
      <div class="card">
        <div class="label">总体统计</div>
        <table>
          <tbody>
            <tr><th>累计局数</th><td id="games">0</td></tr>
            <tr><th>平均分数</th><td id="avgScore">0</td></tr>
            <tr><th>平均时长</th><td id="avgDuration">0秒</td></tr>
            <tr><th>上次结束原因</th><td id="reason"></td></tr>
          </tbody>
        </table>
      </div>
      <div class="card">
        <div class="label">串口事件</div>
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
      document.getElementById('mode').textContent = modeText(c.mode) + ' / ' + stateText(c.state);
      document.getElementById('reason').textContent = reasonText(c.reason);
      document.getElementById('games').textContent = data.summary.games;
      document.getElementById('avgScore').textContent = data.summary.avg_score;
      document.getElementById('avgDuration').textContent = data.summary.avg_duration + '秒';
      document.getElementById('status').textContent = c.last_event || '等待串口事件';

      document.getElementById('ranking').innerHTML = data.rankings.map((row, i) =>
        `<tr><td>${i + 1}</td><td>${modeText(row.mode)}</td><td>${row.score}</td><td>${row.duration}秒</td><td>${row.ended_at_date || ''} ${row.ended_at || ''}</td><td>${reasonText(row.reason)}</td></tr>`
      ).join('') || '<tr><td colspan="6">暂无已结束游戏</td></tr>';

      const maxDuration = Math.max(1, ...data.sessions.map(row => row.duration));
      document.getElementById('bars').innerHTML = data.sessions.slice(-10).map(row => {
        const width = Math.max(3, Math.round(row.duration * 100 / maxDuration));
        return `<div class="bar-row"><span>#${row.index}</span><div class="bar-track"><div class="bar-fill" style="width:${width}%"></div></div><span>${row.duration}秒</span></div>`;
      }).join('') || '<div class="status">暂无时长数据</div>';

      document.getElementById('events').innerHTML = data.events.slice().reverse().map(row =>
        `<div>${row.time} ${row.raw}</div>`
      ).join('');
    }

    function modeText(value) {
      return { normal: '普通', blocks: '障碍' }[value] || value || '';
    }

    function stateText(value) {
      return { idle: '空闲', playing: '游戏中', 'game over': '已结束' }[value] || value || '';
    }

    function reasonText(value) {
      return { wall: '撞墙', body: '撞到自己', block: '撞到障碍', end: '结束' }[value] || value || '';
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
            if not line:
                continue
            parsed = parse_snake_line(line)
            if parsed:
                event_name, fields = parsed
                stats.apply_event(event_name, fields, line)
                print(line)
            else:
                stats.add_serial_line(line)
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
    parser.add_argument(
        "--data-file",
        default=str(Path(__file__).with_name("snake_dashboard_data.json")),
        help="JSON file used to persist finished game records.",
    )
    args = parser.parse_args()
    args.port = normalize_serial_port(args.port)
    return args


def print_available_ports():
    if list_ports is None:
        return

    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports were found.")
        return

    print("Available serial ports:")
    for port in ports:
        print(f"  {port.device}: {port.description}")


def normalize_serial_port(port):
    port = port.strip()
    if port.isdigit():
        return "COM" + port
    return port


def main():
    if serial is None:
        print("pyserial is required. Install it with: pip install pyserial")
        return 1

    args = parse_args()
    stats = SnakeStats(args.data_file)
    stop_event = threading.Event()

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0)
    except serial.SerialException as exc:
        print(f"Could not open serial port {args.port}: {exc}")
        print_available_ports()
        print("Check the board USB cable, driver, Device Manager COM number, and whether another program is using the port.")
        return 1

    with ser:
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
