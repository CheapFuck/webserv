#!/usr/bin/env python3
import os

print("Content-Type: text/html")
print()
print("<html><head><title>Simple CGI Test</title></head><body>")
print("<h1>Simple CGI Test</h1>")
print(f"<p>Request Method: {os.environ.get('REQUEST_METHOD', 'Unknown')}</p>")
print(f"<p>Script Name: {os.environ.get('SCRIPT_NAME', 'Unknown')}</p>")
print("</body></html>")
