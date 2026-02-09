#!/usr/bin/env python3
import os
import cgi

print("Content-Type: text/html\r\n\r\n")
print("<html><head><title>CGI Environment Variables</title></head><body>")
print("<h1>CGI Environment Variables</h1>")
print("<table border='1'><tr><th>Variable</th><th>Value</th></tr>")

for key in sorted(os.environ):
    value = os.environ[key].replace('\n', '<br>').replace('<', '&lt;').replace('>', '&gt;')
    print(f"<tr><td>{key}</td><td>{value}</td></tr>")

print("</table></body></html>")
