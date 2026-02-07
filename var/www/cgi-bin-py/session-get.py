#!/usr/bin/env python3
import os
cookie = os.environ.get('HTTP_COOKIE', 'Aucun cookie')
print("Content-Type: text/html\n")
print(f"<h1>Cookie recu : {cookie}</h1>")
print("<a href='/cgi-bin-py/session-clear.py'>Supprimer</a>")
