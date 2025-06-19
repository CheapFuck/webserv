#!/usr/bin/env python3

import cgilib
import json
import sys
import os

cgi: cgilib.CGIClient = cgilib.CGIClient()

cgi.sendBody("{\"status\": \"ok\", \"message\": \"This is a test message.\"}")