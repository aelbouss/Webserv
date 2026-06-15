#!/usr/bin/python3
import os, sys, html, urllib.parse

method = os.environ.get("REQUEST_METHOD", "GET").upper()
cl     = os.environ.get("CONTENT_LENGTH", "0")
qs     = os.environ.get("QUERY_STRING", "")

params = {}
if method == "POST":
    try:
        length = int(cl)
    except ValueError:
        length = 0
    if length > 0:
        raw = sys.stdin.read(length)
        params = dict(urllib.parse.parse_qsl(raw))
else:
    params = dict(urllib.parse.parse_qsl(qs))

filename = params.get("file", "")
message  = ""
success  = False

if filename:
    # Sanitize: only allow filenames, no path traversal
    safe = os.path.basename(filename)
    target = os.path.join("website/uploads", safe)
    if os.path.isfile(target):
        try:
            os.remove(target)
            message = f"✓ File '{safe}' deleted successfully."
            success = True
        except OSError as e:
            message = f"✗ Error deleting '{safe}': {e}"
    else:
        message = f"✗ File '{safe}' not found in uploads."
else:
    message = "✗ No filename provided. Use ?file=name.txt or POST file=name.txt"

print("Content-Type: text/html\r\n\r")
print(f"""<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><title>Delete CGI</title>
<style>
  body{{font-family:'DM Sans',sans-serif;background:#04080f;color:#e8f0fe;padding:40px;}}
  h1{{font-size:1.8rem;font-weight:800;color:#ff6b6b;margin-bottom:20px;}}
  .ok{{color:#34d399;font-weight:700;font-size:1.1rem;}}
  .err{{color:#ff6b6b;font-weight:700;font-size:1.1rem;}}
  a{{color:#4f8cff;}}
</style></head><body>
<h1>🗑 Delete CGI</h1>
<p class="{'ok' if success else 'err'}">{html.escape(message)}</p>
<p><a href="/pages/upload.html">← Back to Upload</a></p>
</body></html>""")
