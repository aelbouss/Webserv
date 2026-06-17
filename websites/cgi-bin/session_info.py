#!/usr/bin/env python3


import os
import sys
import json
import time

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

def load_session(sid):
    path = os.path.join(SESSION_DIR, f'{sid}.json')
    if not os.path.exists(path):
        return None
    with open(path) as f:
        return json.load(f)

def row(k, v, highlight=False):
    color = '#7c6af7' if highlight else '#aaa'
    return f'<tr><td style="color:{color};padding:7px 12px;white-space:nowrap"><strong>{k}</strong></td><td style="padding:7px 12px;word-break:break-all;color:#ddd">{v}</td></tr>'

def table(rows_html):
    return f'<table style="width:100%;border-collapse:collapse;font-size:.85rem;background:#0d0d1f;border-radius:8px;overflow:hidden">{rows_html}</table>'

#print('Content-Type: text/html; charset=utf-8')
print()

cookies = parse_cookies()
sid     = cookies.get(SESSION_COOKIE, '')
session = load_session(sid) if sid else None
now     = int(time.time())

# ── Cookie section ─────────────────────────────────────────────────────────
cookie_rows = ''
if cookies:
    for k, v in cookies.items():
        is_session = (k == SESSION_COOKIE)
        cookie_rows += row(k, v, highlight=is_session)
else:
    cookie_rows = row('(none)', 'No cookies found in request')

# ── Session section ────────────────────────────────────────────────────────
if session:
    expires_in = session['expires_at'] - now
    exp_str = (
        f'{expires_in // 3600}h {(expires_in % 3600) // 60}m remaining'
        if expires_in > 0 else '<span style="color:#f88">EXPIRED</span>'
    )
    sess_rows = (
        row('session_id',  session['session_id'], True) +
        row('username',    session['username']) +
        row('role',        session['role']) +
        row('display',     session['display']) +
        row('persistent',  'Yes' if session['persistent'] else 'No (session only)') +
        row('created_at',  time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(session['created_at']))) +
        row('expires_at',  time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(session['expires_at'])) + f' ({exp_str})') +
        row('ip',          session['ip']) +
        row('user_agent',  session['user_agent'])
    )
    session_block = f'''
<h3 style="color:#88cc88;margin:24px 0 12px">✅ Active Session</h3>
{table(sess_rows)}
'''
else:
    session_block = '''
<h3 style="color:#cc8888;margin:24px 0 12px">⚠️ No Valid Session</h3>
<p style="color:#888;font-size:.85rem">
  No session cookie found or session file doesn't exist.<br>
  <a href="/login" style="color:#7c6af7">Login first →</a>
</p>
'''

# ── Request env section ────────────────────────────────────────────────────
env_keys = [
    'REQUEST_METHOD','REMOTE_ADDR','HTTP_HOST','HTTP_USER_AGENT',
    'SERVER_PORT','QUERY_STRING','HTTP_COOKIE'
]
env_rows = ''.join(row(k, os.environ.get(k, '(not set)')) for k in env_keys)

print(f'''<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Session &amp; Cookie Inspector</title>
  <style>
    *{{box-sizing:border-box;margin:0;padding:0}}
    body{{font-family:"Segoe UI",sans-serif;background:#0f0f1a;color:#e0e0e0;padding:30px 16px}}
    .wrap{{max-width:700px;margin:0 auto}}
    h2{{color:#7c6af7;margin-bottom:6px}}
    .sub{{color:#555;font-size:.8rem;margin-bottom:28px}}
    h3{{font-size:1rem}}
    tr:nth-child(even){{background:rgba(255,255,255,.03)}}
    a.btn{{display:inline-block;margin:6px 6px 0 0;padding:9px 18px;
           background:#7c6af7;color:#fff;border-radius:8px;text-decoration:none;font-size:.85rem}}
    a.btn.grey{{background:#333}}
    .section{{margin-top:28px}}
  </style>
</head>
<body>
<div class="wrap">
  <h2>🔍 Session &amp; Cookie Inspector</h2>
  <p class="sub">Live view of cookies sent by your browser and the matching server-side session</p>

  <div class="section">
    <h3 style="color:#7c6af7;margin-bottom:12px">🍪 Cookies Received by Server</h3>
    {table(cookie_rows)}
  </div>

  {session_block}

  <div class="section">
    <h3 style="color:#7c6af7;margin-bottom:12px">🌐 CGI Environment</h3>
    {table(env_rows)}
  </div>

  <div style="margin-top:28px">
    <a class="btn" href="/login">Login page</a>
    <a class="btn grey" href="/cgi-bin/logout.py">Logout</a>
    <a class="btn grey" href="/cgi-bin/session_info.py">Refresh</a>
  </div>
</div>
</body></html>''')
