#!/usr/bin/python3
import os, sys, html, urllib.parse

method  = os.environ.get("REQUEST_METHOD", "GET").upper()
qs      = os.environ.get("QUERY_STRING", "")
cl      = os.environ.get("CONTENT_LENGTH", "0")
http_cookie = os.environ.get("HTTP_COOKIE", "")

# Parse existing cookies
cookies = {}
if http_cookie:
    for part in http_cookie.split(";"):
        part = part.strip()
        if "=" in part:
            k, v = part.split("=", 1)
            cookies[k.strip()] = v.strip()

# Read POST body
body_params = {}
if method == "POST":
    try:
        length = int(cl)
    except ValueError:
        length = 0
    if length > 0:
        raw = sys.stdin.read(length)
        body_params = dict(urllib.parse.parse_qsl(raw))

action     = body_params.get("action", qs.split("action=")[-1].split("&")[0] if "action=" in qs else "read")
cookie_name  = body_params.get("name", "webserv_test")
cookie_value = body_params.get("value", "hello42")

set_header = ""
if action == "set":
    set_header = f"Set-Cookie: {cookie_name}={cookie_value}; Path=/; Max-Age=3600\r\n"
elif action == "delete":
    target = body_params.get("name", "webserv_test")
    set_header = f"Set-Cookie: {target}=; Path=/; Max-Age=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n"

print(f"Content-Type: text/html\r\n{set_header}\r")
print("""<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><title>Cookie CGI</title>
<style>
  body{font-family:'DM Sans',sans-serif;background:#04080f;color:#e8f0fe;padding:40px;}
  h1{font-size:1.8rem;font-weight:800;color:#fbbf24;margin-bottom:8px;}
  h2{font-size:1rem;font-weight:700;color:#a78bfa;margin:20px 0 8px;}
  table{width:100%;border-collapse:collapse;margin-bottom:16px;}
  td{padding:9px 14px;border-bottom:1px solid rgba(120,180,255,0.1);font-family:monospace;font-size:.85rem;}
  td:first-child{color:#7a90ab;width:200px;}
  .ok{color:#34d399;font-weight:700;}
  .none{color:#7a90ab;font-style:italic;}
  pre{background:#0a1220;border:1px solid rgba(120,180,255,0.12);border-radius:8px;padding:14px;font-size:.85rem;}
</style></head><body>
<h1>🍪 Cookie CGI</h1>""")

if action == "set":
    print(f'<p class="ok">✓ Set-Cookie header sent for: <code>{html.escape(cookie_name)}={html.escape(cookie_value)}</code></p>')
elif action == "delete":
    target = body_params.get("name", "webserv_test")
    print(f'<p class="ok">✓ Delete-Cookie header sent for: <code>{html.escape(target)}</code></p>')

print("<h2>Current Cookies (from HTTP_COOKIE)</h2>")
if cookies:
    print("<table>")
    for k, v in cookies.items():
        print(f"<tr><td>{html.escape(k)}</td><td>{html.escape(v)}</td></tr>")
    print("</table>")
else:
    print('<p class="none">(no cookies found in request)</p>')

print(f"<h2>Raw HTTP_COOKIE Header</h2>")
print(f"<pre>{html.escape(http_cookie) if http_cookie else '(empty)'}</pre>")
print(f"<h2>Action Performed</h2><p><code>{html.escape(action)}</code></p>")
print("</body></html>")
