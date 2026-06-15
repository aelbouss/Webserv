#!/usr/bin/python3
import os, html

print("Content-Type: text/html\r\n\r")
print("""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8"><title>CGI Environment</title>
<style>
  body{font-family:'DM Sans',sans-serif;background:#04080f;color:#e8f0fe;padding:40px;}
  h1{font-size:1.8rem;font-weight:800;color:#4f8cff;margin-bottom:24px;}
  table{width:100%;border-collapse:collapse;}
  tr:hover{background:rgba(79,140,255,0.04);}
  td{padding:9px 14px;border-bottom:1px solid rgba(120,180,255,0.1);font-family:monospace;font-size:.84rem;}
  td:first-child{color:#a78bfa;width:260px;font-weight:600;}
</style>
</head><body>
<h1>CGI Environment Variables</h1>
<table>""")

for k in sorted(os.environ.keys()):
    v = os.environ[k]
    print(f"<tr><td>{html.escape(k)}</td><td>{html.escape(v)}</td></tr>")

print("</table></body></html>")
