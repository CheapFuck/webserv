#!/usr/bin/env python3

import cgilib
import time
import sys
import os

cgi: cgilib.CGIClient = cgilib.CGIClient()

cgi.setStatus(cgilib.HttpStatusCode.OK)
cgi.setHeader("Content-Type", "application/octet-stream")

CHUNK_SIZE = 1024  # 1MB
TOTAL_SIZE = 1024 * 1024 * 80  # 5GB
chunk = b'\0' * CHUNK_SIZE

written = 0
while written < TOTAL_SIZE:
	to_write = min(CHUNK_SIZE, TOTAL_SIZE - written)
	cgi.sendBody(chunk[:to_write])
	written += to_write
	sys.stdout.flush()
	# time.sleep(0.01)