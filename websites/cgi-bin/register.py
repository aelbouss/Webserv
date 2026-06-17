#!/usr/bin/env python3

import os
import sys
from urllib.parse import parse_qs
import json

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


def html_response(title, body):
    print('Content-Type: text/html; charset=utf-8')
    print()
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
    a{{color:#7c6af7;text-decoration:none}}
    .btn{{display:inline-block;margin-top:16px;padding:10px 20px;
          background:#7c6af7;color:#fff;border-radius:8px;text-decoration:none}}
  </style>
</head>
<body>
<div class="card">
{body}
</div>
</body>
</html>''')


def main():
    method = os.environ.get('REQUEST_METHOD', 'GET').upper()
    if method != 'POST':
        html_response('Method Not Allowed',
            '<h2>405 – Method Not Allowed</h2>'
            '<p>This endpoint only accepts POST requests.</p>'
            '<a class="btn" href="/signup">← Back to signup</a>')
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

    form_data = parse_qs(body, keep_blank_values=True)
    form = {k: v[0] for k, v in form_data.items()}

    username = form.get('username', '').strip()
    password = form.get('password', '').strip()
    display = form.get('display', '').strip() or username

    if not username or not password:
        debug_info = f"<pre style=\"color:#fff;background:#111;border:1px solid #333;padding:12px;border-radius:8px;white-space:pre-wrap;word-break:break-all;\">REQUEST_METHOD={os.environ.get('REQUEST_METHOD')}\nCONTENT_TYPE={os.environ.get('CONTENT_TYPE')}\nCONTENT_LENGTH={os.environ.get('CONTENT_LENGTH')}\nQUERY_STRING={os.environ.get('QUERY_STRING')}\nBODY={body!r}\nPARSED={form!r}</pre>"
        html_response('Missing Fields',
            '<h2>⚠️ Missing information</h2>'
            '<p>Please provide both username and password.</p>'
            '<a class="btn" href="/signup">← Back to signup</a>'
            + debug_info)
        return

    users = load_users()
    if username in users:
        html_response('Already Exists',
            '<h2>⚠️ Username already taken</h2>'
            '<p>Choose a different username.</p>'
            '<a class="btn" href="/signup">← Back to signup</a>')
        return

    users[username] = {
        'password': password,
        'role': 'user',
        'display': display,
    }
    save_users(users)

    html_response('Account Created',
        '<h2>✅ Account Created</h2>'
        '<p>Your new account is ready. Please sign in now.</p>'
        '<a class="btn" href="/login">Go to login</a>')


if __name__ == '__main__':
    main()
