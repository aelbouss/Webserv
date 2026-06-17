#!/usr/bin/env python3

import os
import sys
from urllib.parse import parse_qs
import uuid
import json
import hashlib
import time

# ── Config ──────────────────────────────────────────────────────────────────
SESSION_DIR = os.path.join(os.path.dirname(__file__), '..', 'sessions')
SESSION_COOKIE = 'FUSION_SESSION'
MAX_AGE_PERSISTENT = 7 * 24 * 3600   # 7 days
MAX_AGE_SESSION    = 3600             # 1 hour idle

# ── User database (stored in JSON so signups persist) ──────────────────────
USERS_FILE = os.path.join(os.path.dirname(__file__), 'users.json')
DEFAULT_USERS = {
    'admin': {'password': 'admin123', 'role': 'admin', 'display': 'Administrator'},
    'user': {'password': 'pass456', 'role': 'user', 'display': 'Regular User'},
}

def load_users():
    if not os.path.exists(USERS_FILE):
        save_users(DEFAULT_USERS)
        return dict(DEFAULT_USERS)
    with open(USERS_FILE, 'r') as f:
        return json.load(f)


def save_users(users):
    with open(USERS_FILE, 'w') as f:
        json.dump(users, f, indent=2)

# ── Helpers ──────────────────────────────────────────────────────────────────
def make_session_id():
    return uuid.uuid4().hex

def save_session(sid, data):
    os.makedirs(SESSION_DIR, exist_ok=True)
    path = os.path.join(SESSION_DIR, f'{sid}.json')
    with open(path, 'w') as f:
        json.dump(data, f)

def set_cookie_header(name, value, max_age=None, path='/', http_only=True, same_site='Lax'):
    header = f'Set-Cookie: {name}={value}; Path={path}; SameSite={same_site}'
    if max_age is not None:
        header += f'; Max-Age={max_age}'
    if http_only:
        header += '; HttpOnly'
    return header

def html_response(title, body, extra_headers=None):
    #print('Content-Type: text/html; charset=utf-8')
    if extra_headers:
        for h in extra_headers:
            print(h)
    print()  # blank line ends headers
    print(f'''<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>{title}</title>
  <style>
    *{{box-sizing:border-box;margin:0;padding:0}}
    body{{font-family:"Segoe UI",sans-serif;background:#0f0f1a;color:#e0e0e0;
         min-height:100vh;display:flex;align-items:center;justify-content:center}}
    .card{{background:#1a1a2e;border:1px solid #2a2a4a;border-radius:16px;
           padding:40px;max-width:560px;width:90%;box-shadow:0 8px 40px rgba(0,0,0,.5)}}
    h2{{color:#7c6af7;margin-bottom:20px}}
    .ok{{background:#1a2e1a;border:1px solid #2a4a2a;border-radius:8px;padding:16px;
         color:#88cc88;margin-bottom:16px;line-height:1.7}}
    .err{{background:#2e1a1a;border:1px solid #4a2a2a;border-radius:8px;padding:16px;
          color:#cc8888;margin-bottom:16px}}
    .kv{{display:grid;grid-template-columns:auto 1fr;gap:6px 14px;
         font-size:.85rem;margin-top:10px}}
    .k{{color:#7c6af7;font-weight:600;white-space:nowrap}}
    .v{{color:#ccc;word-break:break-all}}
    a{{color:#7c6af7;text-decoration:none}}
    a:hover{{text-decoration:underline}}
    .btn{{display:inline-block;margin-top:16px;padding:10px 20px;
          background:#7c6af7;color:#fff;border-radius:8px;text-decoration:none}}
  </style>
</head>
<body>
<div class="card">
{body}
</div>
</body></html>''')

def redirect_response(location, extra_headers=None):
    print('Status: 303 See Other')
    print(f'Location: {location}')
    if extra_headers:
        for h in extra_headers:
            print(h)
    print()

# ── Main ─────────────────────────────────────────────────────────────────────
def main():
    method = os.environ.get('REQUEST_METHOD', 'GET').upper()

    if method != 'POST':
        html_response('Method Not Allowed',
            '<h2>405 – Method Not Allowed</h2>'
            '<p>This endpoint only accepts POST requests.</p>'
            '<a href="/login">← Back to login</a>')
        return

    length = 0
    try:
        length = int(os.environ.get('CONTENT_LENGTH') or 0)
    except (TypeError, ValueError):
        length = 0

    body = ''
    if length:
        try:
            body = os.read(0, length).decode('utf-8', 'replace')
        except Exception:
            try:
                body = sys.stdin.buffer.read(length).decode('utf-8', 'replace')
            except Exception:
                body = ''

        if not body:
            try:
                body = sys.stdin.read(length)
            except Exception:
                body = ''
    else:
        try:
            body = sys.stdin.read()
        except Exception:
            body = ''

    if not body.strip():
        body = os.environ.get('REQUEST_BODY', '')
    if not body.strip():
        body = os.environ.get('HTTP_POST_DATA', '')
    if not body.strip():
        body = os.environ.get('POST_DATA', '')
    if not body.strip():
        body = os.environ.get('QUERY_STRING', '')

    form = {k: v[0] for k, v in parse_qs(body, keep_blank_values=True).items()}

    username = form.get('username', '').strip()
    password = form.get('password', '').strip()
    remember  = form.get('remember', '0') == '1'

    # ── Validate credentials ─────────────────────────────────────────────────
    users = load_users()
    user = users.get(username)
    if not user or user['password'] != password:
        html_response('Login Failed',
            '<h2>🔴 Login Failed</h2>'
            '<div class="err">Invalid username or password.</div>'
            '<a href="/login">← Try again</a>')
        return

    # ── Create session ───────────────────────────────────────────────────────
    sid = make_session_id()
    now = int(time.time())
    session_data = {
        'session_id':  sid,
        'username':    username,
        'role':        user['role'],
        'display':     user['display'],
        'created_at':  now,
        'expires_at':  now + (MAX_AGE_PERSISTENT if remember else MAX_AGE_SESSION),
        'persistent':  remember,
        'ip':          os.environ.get('REMOTE_ADDR', 'unknown'),
        'user_agent':  os.environ.get('HTTP_USER_AGENT', 'unknown'),
    }
    save_session(sid, session_data)

    # ── Set cookies ──────────────────────────────────────────────────────────
    cookies = []
    # 1. Session ID cookie (always set)
    cookies.append(set_cookie_header(
        SESSION_COOKIE, sid,
        max_age=(MAX_AGE_PERSISTENT if remember else None)
    ))
    # 2. Username display cookie (NOT HttpOnly so JS can read it)
    cookies.append(
        f'Set-Cookie: fusion_user={username}; Path=/; SameSite=Lax'
        + (f'; Max-Age={MAX_AGE_PERSISTENT}' if remember else '')
    )
    # 3. Role cookie (plain, for demo purposes)
    cookies.append(
        f'Set-Cookie: fusion_role={user["role"]}; Path=/; SameSite=Lax'
        + (f'; Max-Age={MAX_AGE_PERSISTENT}' if remember else '')
    )

    # ── Build success page ───────────────────────────────────────────────────
    expire_label = (
        f'in 7 days ({time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(session_data["expires_at"]))})'
        if remember else
        'when browser closes (session cookie)'
    )

    redirect_response('/cgi-bin/session_info.py', cookies)

main()
