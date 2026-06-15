#!/usr/bin/python3
import os, html, urllib.parse

qs = os.environ.get("QUERY_STRING", "")
params = dict(urllib.parse.parse_qsl(qs))

a_str  = params.get("a", "")
op     = params.get("op", "+")
b_str  = params.get("b", "")

result = ""
error  = ""

try:
    a = float(a_str)
    b = float(b_str)
    if op == "+":   result = a + b
    elif op == "-": result = a - b
    elif op == "*": result = a * b
    elif op == "/":
        if b == 0: error = "Division by zero"
        else: result = a / b
    else:
        error = f"Unknown operator: {op}"
    if not error:
        result = int(result) if result == int(result) else result
except ValueError:
    error = f"Invalid numbers: a='{a_str}' b='{b_str}'"

print("Content-Type: text/html\r\n\r")
print(f"""<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><title>Calc CGI</title>
<style>
  body{{font-family:'DM Sans',sans-serif;background:#04080f;color:#e8f0fe;padding:40px;}}
  h1{{font-size:1.8rem;font-weight:800;color:#4f8cff;margin-bottom:24px;}}
  .expr{{font-size:2.5rem;font-family:monospace;margin-bottom:16px;}}
  .ans{{font-size:3rem;font-weight:800;color:#34d399;}}
  .err{{color:#ff6b6b;font-size:1.2rem;}}
  p{{color:#7a90ab;margin-top:12px;font-size:.9rem;}}
</style></head><body>
<h1>Calculator CGI</h1>
<div class="expr">{html.escape(a_str)} {html.escape(op)} {html.escape(b_str)}</div>
""")

if error:
    print(f'<div class="err">Error: {html.escape(error)}</div>')
else:
    print(f'<div class="ans">= {result}</div>')

print(f'<p>Query string: <code>{html.escape(qs)}</code></p>')
print("</body></html>")
