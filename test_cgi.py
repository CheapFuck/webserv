#!/usr/bin/env python3
import os
import sys
import cgi

# Print headers
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print HTML content
print("<html><head><title>Python CGI Test</title></head><body>")
print("<h1>Python CGI Test</h1>")

print(f"<p>Request Method: {os.environ.get('REQUEST_METHOD', 'Unknown')}</p>")
print(f"<p>Script Name: {os.environ.get('SCRIPT_NAME', 'Unknown')}</p>")
print(f"<p>Query String: {os.environ.get('QUERY_STRING', 'Unknown')}</p>")
print(f"<p>Server Name: {os.environ.get('SERVER_NAME', 'Unknown')}</p>")
print(f"<p>Server Port: {os.environ.get('SERVER_PORT', 'Unknown')}</p>")

if os.environ.get('REQUEST_METHOD') == 'POST':
    print("<h2>POST Data:</h2>")
    print("<pre>")
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        print(cgi.escape(post_data))
    print("</pre>")

print("<h2>Environment Variables:</h2>")
print("<pre>")
for key, value in sorted(os.environ.items()):
    print(f"{cgi.escape(key)} = {cgi.escape(value)}")
print("</pre>")

print("</body></html>")
