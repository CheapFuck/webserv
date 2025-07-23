#!/usr/bin/env python3

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import cgilib
import json

cgi: cgilib.CGIClient = cgilib.CGIClient()

@cgi.route('/zen')
def createZenResponse():
    with open('zenOfPython', 'r') as f:
        lines = f.read().splitlines()
    
    try: amount_of_lines = cgi.session.get('zenAmountOfLines', 1)
    except ValueError: amount_of_lines = 1

    cgi.setHeader("Content-Type", "text/plain; charset=utf-8")
    cgi.setStatus(cgilib.HttpStatusCode.OK)

    if amount_of_lines <= len(lines):
        cgi.sendBody("\n".join(lines[:amount_of_lines]))
        cgi.session['zenAmountOfLines'] = amount_of_lines + 1
    else:
        cgi.sendBody("You have reached the end of the Zen of Python.\nReload to start over.\n")
        cgi.session['zenAmountOfLines'] = 1

@cgi.route('/tea')
def createTeapotResponse():
    cgi.setHeader("Content-Type", "text/plain; charset=utf-8")
    cgi.setStatus(cgilib.HttpStatusCode.ImATeapot)
    cgi.sendBody("I'm a teapot, not a web server!\n")

@cgi.route('/')
def createDebugResponse():
    cgi.setHeader("Content-Type", "application/json; charset=utf-8")
    cgi.setStatus(cgilib.HttpStatusCode.OK)

    cgi.sendBody(json.dumps({
        "script": cgi.getEnvironmentVariable('SCRIPT_NAME', '<empty>'),
        "path": cgi.getEnvironmentVariable('PATH_INFO', '<empty>'),
        "query": cgi.getEnvironmentVariable('QUERY_STRING', '<empty>'),
        "body": cgi.getBody(),
        "sessionData": dict(cgi.session),
        "envVars": dict(os.environ),
    }))

with cgi.session:
    debugVisitCounter = int(cgi.session.get("debugVisitCounter", 0)) + 1
    cgi.session["debugVisitCounter"] = debugVisitCounter
    cgi.run()