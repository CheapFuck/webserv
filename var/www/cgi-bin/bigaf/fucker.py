#!/usr/bin/env python3

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import cgilib
import time
import math

cgi: cgilib.CGIClient = cgilib.CGIClient()

cgi.setStatus(cgilib.HttpStatusCode.OK)
cgi.setHeader("Content-Type", "application/octet-stream")

size_to_send_in_bytes = math.floor(float(cgi.getPathParameter(0) or 0.5) * 1024 * 1024 * 1024)  # Default to 0.5 GB if not specified 
should_be_slowed: bool = bool(cgi.getQueryParameter('slow'))

CHUNK_SIZE = 1024  # 1MB
chunk = '0' * CHUNK_SIZE

written = 0
while written < size_to_send_in_bytes:
	to_write = min(CHUNK_SIZE, size_to_send_in_bytes - written)
	cgi.sendBody(chunk[:to_write])
	written += to_write
	# sys.stdout.flush()

	if should_be_slowed:
		time.sleep(0.001)