#!/usr/bin/python3
import os, sys, html

method  = os.environ.get("REQUEST_METHOD", "GET").upper()
query   = os.environ.get("QUERY_STRING", "")
ctype   = os.environ.get("CONTENT_TYPE", "")
cookie  = os.environ.get("HTTP_COOKIE", "")
cl      = os.environ.get("CONTENT_LENGTH", "0")
ua      = os.environ.get("HTTP_USER_AGENT", "")
host    = os.environ.get("SERVER_NAME", "")
port    = os.environ.get("SERVER_PORT", "")
uri     = os.environ.get("REQUEST_URI", "")

body = ""
if method == "POST":
    try:
        length = int(cl)
    except ValueError:
        length = 0
    if length > 0:
        body = sys.stdin.read(length)

env_vars = {k: v for k, v in os.environ.items()}

print("Content-Type: text/html\r\n\r")
print("""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Echo CGI</title>
<style>
  body{font-family:'DM Sans',sans-serif;background:#04080f;color:#e8f0fe;padding:40px;line-height:1.6;}
  h1{font-size:1.8rem;font-weight:800;margin-bottom:8px;color:#4f8cff;}
  h2{font-size:1rem;font-weight:700;color:#a78bfa;margin:24px 0 8px;}
  table{width:100%;border-collapse:collapse;margin-bottom:16px;}
  td{padding:8px 12px;border-bottom:1px solid rgba(120,180,255,0.1);font-family:monospace;font-size:.85rem;}
  td:first-child{color:#7a90ab;width:200px;}
  pre{background:#0a1220;border:1px solid rgba(120,180,255,0.12);border-radius:8px;padding:16px;font-size:.85rem;white-space:pre-wrap;word-break:break-all;}
  .badge{display:inline-block;padding:3px 10px;border-radius:999px;font-size:.75rem;font-weight:700;background:rgba(79,140,255,.15);color:#4f8cff;}
</style>
</head>
<body>
<h1>⚡ Echo CGI</h1>
<span class="badge">""" + method + """</span>""")

print("<h2>Request Line</h2><table>")
print(f"<tr><td>METHOD</td><td>{html.escape(method)}</td></tr>")
print(f"<tr><td>REQUEST_URI</td><td>{html.escape(uri)}</td></tr>")
print(f"<tr><td>QUERY_STRING</td><td>{html.escape(query) if query else '(empty)'}</td></tr>")
print(f"<tr><td>CONTENT_TYPE</td><td>{html.escape(ctype) if ctype else '(none)'}</td></tr>")
print(f"<tr><td>CONTENT_LENGTH</td><td>{html.escape(cl)}</td></tr>")
print(f"<tr><td>HOST</td><td>{html.escape(host)}:{html.escape(port)}</td></tr>")
print("</table>")

if cookie:
    print("<h2>Cookies (HTTP_COOKIE)</h2><pre>" + html.escape(cookie) + "</pre>")
else:
    print("<h2>Cookies</h2><pre>(no cookies sent)</pre>")

if body:
    print("<h2>Request Body</h2><pre>" + html.escape(body) + "</pre>")

print("<h2>All CGI Environment Variables</h2><table>")
for k in sorted(env_vars.keys()):
    print(f"<tr><td>{html.escape(k)}</td><td>{html.escape(env_vars[k])}</td></tr>")
print("</table>")

print("</body></html>")
