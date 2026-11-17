#!/usr/bin/env python3
"""Synthetic loopback admin server shared by independent HTTP and browser tests.
No real credentials, Google/YouTube requests, or application state.
"""
import contextlib
import http.server
import json
import pathlib
import secrets
import socket
import subprocess
import threading
import time
import urllib.parse

HERE = pathlib.Path(__file__).resolve().parent
SYNTHETIC_USER = "fixture_operator"
ASSET_COUNT = 8
ASSET_BYTES = 256 * 1024

def private_disk_usage(path):
    return sum(p.stat().st_blocks * 512 for p in pathlib.Path(path).rglob("*")
               if p.is_file())


class FixtureState:
    def __init__(self):
        self.password = secrets.token_urlsafe(22)
        self.session = secrets.token_urlsafe(23)
        self.created = {}
        self.post_attempts = {}
        self.body_bytes = 0
        self.responses = 0
        self.lock = threading.Lock()

    def response(self, count):
        with self.lock:
            self.body_bytes += count
            self.responses += 1

    def attempts(self, operation):
        with self.lock:
            return self.post_attempts.get(operation, 0)


HTML_LOGIN = """<!doctype html><html><head><meta charset='utf-8'></head><body>
<h1>Fixture authentication</h1><form method='POST' action='/login'>
<label>Account<input name='account' id='account'></label>
<label>Password<input name='password' id='password' type='password'></label>
<button type='submit'>Sign in</button></form></body></html>"""


def admin_html():
    images = "\n".join(f"<img class='irrelevant' src='/asset/{i}' width='320' height='100' alt=''>"
                       for i in range(ASSET_COUNT))
    return f"""<!doctype html><html><head><meta charset='utf-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<title>Synthetic API key administration</title>
<style>body{{font:16px sans-serif}} .irrelevant{{display:block;width:320px;height:100px}}</style>
</head><body><h1>API key control</h1>
<table id='useful-table'><tr><th>Scope</th><th>Status</th></tr>
<tr><td>Local fixture</td><td>Ready</td></tr></table>
<label>Ordinary key label<input id='label' autocomplete='off'></label>
<button id='create' type='button'>Create one-time key</button>
<output id='status' role='status'></output>
<output id='once-only-key'></output>
<script>
document.getElementById('create').addEventListener('click', async () => {{
  const label = document.getElementById('label').value;
  const operation = document.getElementById('create').dataset.operation;
  if (!operation) {{document.getElementById('status').textContent='No browser-owned authorization'; return;}}
  document.getElementById('create').disabled = true;
  try {{
    const response = await fetch('/api/create', {{method:'POST',credentials:'same-origin',
      headers:{{'Content-Type':'application/json'}},body:JSON.stringify({{label,operation}})}});
    if (!response.ok) throw new Error('HTTP failure');
    const result = await response.json();
    document.getElementById('once-only-key').textContent = result.key;
    document.getElementById('status').textContent = 'Created';
  }} catch (e) {{document.getElementById('status').textContent = 'Unknown outcome; do not retry';}}
}});
</script><section id='bulky-media'>{images}</section></body></html>"""


class FixtureHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, *args):
        pass  # No URL, cookie, request body, password or key logging.

    @property
    def state(self):
        return self.server.state

    def authorized(self):
        cookie = self.headers.get("Cookie", "")
        return cookie == "ib_session=" + self.state.session

    def send_bytes(self, code, body, mime="text/html; charset=utf-8", headers=None):
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", mime)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        if headers:
            for k, v in headers.items():
                self.send_header(k, v)
        self.end_headers()
        try:
            self.wfile.write(body)
            self.wfile.flush()
            self.state.response(len(body))
        except (BrokenPipeError, ConnectionResetError):
            pass

    def redirect(self, target):
        self.send_bytes(303, "", headers={"Location": target})

    def do_GET(self):
        route = urllib.parse.urlsplit(self.path)
        if route.path == "/login":
            self.send_bytes(200, HTML_LOGIN)
        elif route.path == "/admin":
            if not self.authorized():
                self.redirect("/login")
            else:
                self.send_bytes(200, admin_html())
        elif route.path == "/verification":
            self.send_bytes(200, "<h1 id='verification'>Verification interruption</h1>")
        elif route.path.startswith("/asset/"):
            # Real over-the-socket irrelevant payload; browser progress must not
            # wait for all of it. Delays exist in the fixture, not production.
            try:
                index = int(route.path[len("/asset/"):])
            except ValueError:
                index = -1
            if not (0 <= index < ASSET_COUNT):
                self.send_bytes(404, "No asset")
                return
            prefix = f"<svg xmlns='http://www.w3.org/2000/svg'><desc>decorative-{index}-".encode()
            suffix = b"</desc></svg>"
            body = prefix + (b"x" * (ASSET_BYTES - len(prefix) - len(suffix))) + suffix
            self.send_response(200)
            self.send_header("Content-Type", "image/svg+xml")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            try:
                for position in range(0, len(body), 4096):
                    self.wfile.write(body[position:position + 4096])
                    self.wfile.flush()
                    time.sleep(0.003)
                self.state.response(len(body))
            except (BrokenPipeError, ConnectionResetError):
                pass
        elif route.path == "/api/status":
            if not self.authorized():
                self.send_bytes(401, "Denied")
                return
            operation = urllib.parse.parse_qs(route.query).get("operation", [""])[0]
            with self.state.lock:
                applied = operation in self.state.created
            payload = json.dumps({"applied": applied, "operation": operation})
            self.send_bytes(200, payload, "application/json")
        else:
            self.send_bytes(404, "Unknown")

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        if length > 2048:
            self.send_bytes(413, "Too large")
            return
        payload = self.rfile.read(length)
        if self.path == "/login":
            form = urllib.parse.parse_qs(payload.decode("utf-8"))
            if form.get("account") == [SYNTHETIC_USER] and form.get("password") == [self.state.password]:
                self.redirect_with_session("/admin")
            else:
                self.send_bytes(401, "Denied")
            return
        if self.path != "/api/create" or not self.authorized():
            self.send_bytes(401, "Denied")
            return
        try:
            body = json.loads(payload.decode("utf-8"))
            operation, label = body["operation"], body["label"]
            if operation not in ("opSuccess", "opAmbiguous") or label not in ("OrdinaryOne", "OrdinaryTwo"):
                raise ValueError("Unknown fixture operation")
        except (ValueError, KeyError, TypeError):
            self.send_bytes(400, "Rejected")
            return
        with self.state.lock:
            self.state.post_attempts[operation] = self.state.post_attempts.get(operation, 0) + 1
            duplicate = operation in self.state.created
            if not duplicate:
                # The one-time secret exists ONLY in server memory and, if
                # delivered, in the live browser's transient DOM.
                key = "synthetic_" + secrets.token_urlsafe(24)
                self.state.created[operation] = {"label": label, "one_time_secret": key}
        # Do not write a response while holding the lock: send_bytes itself
        # records byte counts under the same lock. This lets a deliberately
        # replayed POST be rejected promptly rather than hanging the observer.
        if duplicate:
            self.send_bytes(409, "Already applied")
            return
        if operation == "opAmbiguous":
            # Mutate server state and then sever the connection before delivery.
            # The client can never assume absence of effect.
            self.close_connection = True
            with contextlib.suppress(OSError):
                self.connection.shutdown(socket.SHUT_RDWR)
            with contextlib.suppress(OSError):
                self.connection.close()
            return
        self.send_bytes(201, json.dumps({"key": key}), "application/json")

    def redirect_with_session(self, target):
        self.send_bytes(303, "", headers={
            "Location": target,
            "Set-Cookie": f"ib_session={self.state.session}; HttpOnly; SameSite=Strict; Path=/"
        })


class FixtureServer(http.server.ThreadingHTTPServer):
    daemon_threads = True
    def __init__(self, state):
        super().__init__(("127.0.0.1", 0), FixtureHandler)
        self.state = state


def call_journal(driver, root, task, action, parameter=None, expected=0):
    command = [driver, root, task, action]
    if parameter is not None:
        command.append(parameter)
    invocation = subprocess.run(command, check=False, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert invocation.returncode == expected, (action, invocation.returncode)
    return invocation.stdout.strip()
