#!/usr/bin/env python3

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import cgilib
import json

cgi: cgilib.CGIClient = cgilib.CGIClient()

def createZenResponse():
    with open('zenOfPython', 'r') as f:
        zen = f.read()

    cgi.setHeader("Content-Type", "text/plain; charset=utf-8")
    cgi.setStatus(cgilib.HttpStatusCode.OK)
    cgi.sendBody(zen)

def createDebugResponse():
    cgi.setHeader("Content-Type", "application/json; charset=utf-8")
    cgi.setStatus(cgilib.HttpStatusCode.OK)

    cgi.sendBody(json.dumps({
        "script": cgi.getEnvironmentVariable('SCRIPT_NAME', '?'),
        "path": cgi.getEnvironmentVariable('PATH_INFO', '?'),
        "query": cgi.getEnvironmentVariable('QUERY_STRING', '?'),
        "sessionData": dict(cgi.session),
        "envVars": dict(os.environ),
    }))

with cgi.session:
    debugVisitCounter = int(cgi.session.get("debugVisitCounter", 0)) + 1
    cgi.session["debugVisitCounter"] = debugVisitCounter

    if cgi.getEnvironmentVariable('PATH_INFO', '') == '/zen':
        createZenResponse()
    else:
        createDebugResponse()
