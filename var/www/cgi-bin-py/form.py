#!/usr/bin/env python3
import sys
import os

content_length = int(os.environ.get('CONTENT_LENGTH', 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""

print("Content-Type: text/html\r\n\r\n", end="")
print("<html><body>")
print("<h1>CGI Form Handler</h1>")
print(f"<p>Received: {body}</p>")
print("</body></html>")
