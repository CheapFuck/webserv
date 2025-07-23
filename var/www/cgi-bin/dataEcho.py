#!/usr/bin/env python3

import cgilib

cgi: cgilib.CGIClient = cgilib.CGIClient()

cgi.setStatus(cgilib.HttpStatusCode.OK)

cgi.sendBody(cgi.getBody() or '')