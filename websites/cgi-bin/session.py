#!/usr/bin/python3
import os, sys, html, urllib.parse

method      = os.environ.get("REQUEST_METHOD", "GET").upper()
cl          = os.environ.get("CONTENT_LENGTH", "0")
http_cookie = os.environ.get("HTTP_COOKIE", "")

# Parse cookies
cookies = {}
if http_cookie:
    for part in http_cookie.split(";"):
        part = part.strip()
        if "=" in part:
            k, v = part.split("=", 1)
            cookies[k.strip()] = v.strip()

sid = cookies.get("WEBSERVSID", "")

# Parse POST body
body_params = {}
if method == "POST":
    try:
        length = int(cl)
    except ValueError:
        length = 0
    if length > 0:
        raw = sys.stdin.read(length)
        body_params = dict(urllib.parse.parse_qsl(raw))

action   = body_params.get("action", "status")
username = body_params.get("username", "")
password = body_params.get("password", "")

VALID_USERS = {"admin": "1337", "student": "42", "tester": "webserv"}

extra_header = ""
message = ""
logged_in = bool(sid)

if action == "login":
    if username in VALID_USERS and VALID_USERS[username] == password:
        # Server would normally create a real session — we simulate with a fake SID
        import hashlib, time
        fake_sid = hashlib.sha256(f"{username}{time.time()}".encode()).hexdigest()[:32]
        extra_header = f"Set-Cookie: WEBSERVSID={fake_sid}; Path=/; HttpOnly; SameSite=Lax; Max-Age=3600\r\n"
        extra_header += f"Set-Cookie: webserv_user={username}; Path=/; Max-Age=3600\r\n"
        message = f"✓ Login successful! Welcome, {username}. WEBSERVSID set."
        logged_in = True
        sid = fake_sid
    else:
        message = "✗ Invalid credentials. Try: admin/1337, student/42, tester/webserv"

elif action == "logout":
    extra_header  = "Set-Cookie: WEBSERVSID=; Path=/; Max-Age=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n"
    extra_header += "Set-Cookie: webserv_user=; Path=/; Max-Age=0\r\n"
    message = "✓ Logged out. Session cookie cleared."
    logged_in = False
    sid = ""

print(f"Content-Type: text/html\r\n{extra_header}\r")
print("""<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><title>Session CGI</title>
<style>
  body{font-family:'DM Sans',sans-serif;background:#04080f;color:#e8f0fe;padding:40px;max-width:700px;margin:0 auto;}
  h1{font-size:1.8rem;font-weight:800;color:#34d399;margin-bottom:8px;}
  h2{font-size:1rem;font-weight:700;color:#a78bfa;margin:24px 0 8px;}
  .ok{color:#34d399;font-weight:700;margin-bottom:16px;}
  .err{color:#ff6b6b;font-weight:700;margin-bottom:16px;}
  input{width:100%;padding:10px 14px;background:#0a1220;border:1px solid rgba(120,180,255,0.12);border-radius:8px;color:#e8f0fe;font-size:.95rem;margin-bottom:12px;outline:none;}
  button{padding:10px 24px;background:#4f8cff;color:#fff;border:none;border-radius:8px;font-weight:700;cursor:pointer;margin-right:8px;}
  button.danger{background:#ff6b6b;}
  table{width:100%;border-collapse:collapse;margin-bottom:16px;}
  td{padding:9px 14px;border-bottom:1px solid rgba(120,180,255,0.1);font-family:monospace;font-size:.85rem;}
  td:first-child{color:#7a90ab;width:180px;}
  .sid{word-break:break-all;color:#fbbf24;}
  .status-on{color:#34d399;font-weight:700;}
  .status-off{color:#7a90ab;}
  pre{background:#0a1220;border:1px solid rgba(120,180,255,0.12);border-radius:8px;padding:14px;font-size:.84rem;word-break:break-all;}
</style></head><body>
<h1>🔐 Session CGI</h1>""")

if message:
    cls = "ok" if "✓" in message else "err"
    print(f'<p class="{cls}">{html.escape(message)}</p>')

print(f"<h2>Session Status</h2>")
print(f'<p>Status: <span class="{"status-on" if logged_in else "status-off"}">{"● Logged in" if logged_in else "○ Not logged in"}</span></p>')
if sid:
    print(f'<p>WEBSERVSID: <span class="sid">{html.escape(sid)}</span></p>')
user = cookies.get("webserv_user", "")
if user:
    print(f'<p>User: <strong>{html.escape(user)}</strong></p>')

print("""<h2>Login</h2>
<form method="POST" action="/cgi-bin/session.py">
  <input type="hidden" name="action" value="login">
  <input type="text" name="username" placeholder="Username (admin / student / tester)">
  <input type="password" name="password" placeholder="Password (1337 / 42 / webserv)">
  <button type="submit">Login</button>
</form>""")

if logged_in:
    print("""<h2>Logout</h2>
<form method="POST" action="/cgi-bin/session.py">
  <input type="hidden" name="action" value="logout">
  <button type="submit" class="danger">Logout</button>
</form>""")

print(f"<h2>All Cookies in Request</h2>")
if cookies:
    print("<table>")
    for k, v in cookies.items():
        print(f"<tr><td>{html.escape(k)}</td><td>{html.escape(v)}</td></tr>")
    print("</table>")
else:
    print("<p style='color:#7a90ab'>(no cookies)</p>")

print(f"<h2>Raw HTTP_COOKIE</h2><pre>{html.escape(http_cookie) if http_cookie else '(empty)'}</pre>")
print("</body></html>")
