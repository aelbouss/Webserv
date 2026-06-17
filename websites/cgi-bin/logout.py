#!/usr/bin/env python3
import os
import json

SESSION_DIR    = os.path.join(os.path.dirname(__file__), '..', 'sessions')
SESSION_COOKIE = 'FUSION_SESSION'

def parse_cookies():
    raw = os.environ.get('HTTP_COOKIE', '')
    cookies = {}
    for part in raw.split(';'):
        part = part.strip()
        if '=' in part:
            k, _, v = part.partition('=')
            cookies[k.strip()] = v.strip()
    return cookies

cookies = parse_cookies()
sid     = cookies.get(SESSION_COOKIE, '')

# Delete session file
deleted = False
if sid:
    path = os.path.join(SESSION_DIR, f'{sid}.json')
    if os.path.exists(path):
        os.remove(path)
        deleted = True

# Print headers — expire all cookies immediately (Max-Age=0)
#print('Content-Type: text/html; charset=utf-8')
print(f'Set-Cookie: {SESSION_COOKIE}=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax')
print('Set-Cookie: fusion_user=; Path=/; Max-Age=0; SameSite=Lax')
print('Set-Cookie: fusion_role=; Path=/; Max-Age=0; SameSite=Lax')
print()

print(f'''<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta http-equiv="refresh" content="3;url=/login">
  <title>Logged Out</title>
  <style>
    *{{box-sizing:border-box;margin:0;padding:0}}
    body{{font-family:"Segoe UI",sans-serif;background:#0f0f1a;color:#e0e0e0;
         min-height:100vh;display:flex;align-items:center;justify-content:center}}
    .card{{background:#1a1a2e;border:1px solid #2a2a4a;border-radius:16px;
           padding:40px;max-width:420px;width:90%;text-align:center}}
    h2{{color:#7c6af7;margin-bottom:16px}}
    p{{color:#888;font-size:.9rem;line-height:1.7;margin-bottom:20px}}
    a{{color:#7c6af7;text-decoration:none}}
  </style>
</head>
<body>
<div class="card">
  <h2>👋 Logged Out</h2>
  <p>
    Session {"deleted from server" if deleted else "was already gone"}.<br>
    All cookies cleared (Max-Age=0).<br><br>
    Redirecting to login in 3 seconds…
  </p>
  <a href="/login">← Go to login now</a>
</div>
</body></html>''')
