#!/usr/bin/env python3
import sys
import os
import time

time.sleep(10)
body = sys.stdin.read()

print("Content-Type: text/html")
print()
print("<h1>Hello from CGI</h1>")

sys.stdout.flush()
