#!/usr/bin/python3
import os, sys

method = os.environ.get("REQUEST_METHOD", "GET")
query  = os.environ.get("QUERY_STRING", "")
clen   = os.environ.get("CONTENT_LENGTH", "0")

body = ""
if method in ("POST", "PUT"):
    try:
        n = int(clen)
    except ValueError:
        n = 0
    if n > 0:
        body = sys.stdin.read(n)

http_headers = []
for k, v in sorted(os.environ.items()):
    if k.startswith("HTTP_"):
        http_headers.append("  %s: %s" % (k[5:].replace("_", "-").title(), v))

out = []
out.append("=== CGI echo ===")
out.append("Method : %s" % method)
out.append("Query  : %s" % (query or "(none)"))
out.append("Request headers seen by the script:")
out.extend(http_headers or ["  (none)"])
out.append("")
out.append("Body (%s bytes):" % len(body))
out.append(body if body else "(empty)")
payload = "\n".join(out) + "\n"

sys.stdout.write("Content-Type: text/plain; charset=utf-8\r\n")
sys.stdout.write("X-CGI-Script: echo.py\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(payload)
