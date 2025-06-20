#!/usr/bin/env python3

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import cgilib
import json

cgi: cgilib.CGIClient = cgilib.CGIClient()

cgi.setHeader("Content-Type", "application/json; charset=utf-8")
cgi.setStatus(cgilib.HttpStatusCode.OK)

cgi.sendBody(json.dumps({
    "user": "someUser",
    "url_params": cgi.getEnvironmentVariable("PATH_INFO", ""),
    "envVars": dict(os.environ)
}))