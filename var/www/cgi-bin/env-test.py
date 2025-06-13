#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/plain")
print()

print("Python CGI Environment Variables:")
for key, value in sorted(os.environ.items()):
    print(f"{key}: {value}")

print("\nPython version:")
print(sys.version)

print("\nScript arguments:")
print(sys.argv)