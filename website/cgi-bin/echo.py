#!/usr/bin/python3

import os
import sys

method = os.environ.get("REQUEST_METHOD", "GET").upper()
query = os.environ.get("QUERY_STRING", "")
content_length = os.environ.get("CONTENT_LENGTH", "0")
body = ""

if method == "POST":
    try:
        length = int(content_length)
    except ValueError:
        length = 0
    if length > 0:
        body = sys.stdin.read(length)

print("Content-Type: text/html\r\n\r\n")
print("<html>")
print("<head><title>Echo CGI</title></head>")
print("<body>")
print("<h1>Echo CGI</h1>")
print("<p>Method: %s</p>" % method)
print("<p>Query: %s</p>" % (query if query else "(empty)"))
if body:
    print("<pre>%s</pre>" % body)
print("</body>")
print("</html>")