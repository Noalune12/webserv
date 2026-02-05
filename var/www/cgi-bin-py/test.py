#!/usr/bin/env python3
import sys
import os

body = sys.stdin.read()

print("Content-Type: text/html")
print()
print("<h1>Hello from CGI</h1>")

sys.stdout.flush()
