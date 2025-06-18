#!/usr/bin/env python3

import cgilib

cgi: cgilib.CGIClient = cgilib.CGIClient()

with cgi.session:
    cgi.session["me"] = {
        "name": "Test User",
        "email": "somemail@gmail.com",
        "roles": ["admin", "user"],
        "preferences": {
            "theme": "dark",
            "notifications": True,
            "language": "en-US"
        }
    }

cgi.setHeader("Content-Type", "text/plain; charset=utf-8")
cgi.setHeader("X-Content-Type-Options", "nosniff")
cgi.setHeader("X-Frame-Options", "DENY")
cgi.setHeader("X-XSS-Protection", "1; mode=block")
cgi.setHeader("X-Content-Security-Policy", "default-src 'none'; script-src 'self'; style-src 'self'; img-src 'self' data:; font-src 'self'; frame-ancestors 'none'; base-uri 'self'; form-action 'self'")
cgi.setHeader("X-Permitted-Cross-Domain-Policies", "none")
cgi.setHeader("X-Download-Options", "noopen")
cgi.setHeader("X-Content-Type-Options", "nosniff")
cgi.setHeader("X-Frame-Options", "DENY")
cgi.setHeader("X-XSS-Protection", "1; mode=block")

somebody = cgi.getBody()
with cgi.session:
    cgi.session["body"] = somebody

cgi.sendBody(f"Hello, {cgi.session['me']}!\n")
cgi.sendBody("This is a test of the CGI environment.\n")
cgi.sendBody("You can access environment variables like this:\n")
cgi.sendBody(f"HTTP_USER_AGENT: {cgi.getEnvironmentVariable('HTTP_USER_AGENT', 'Unknown')}\n")
cgi.sendBody("And cookies like this:\n")
cgi.sendBody(f"CGI_COOKIE_TEST: {cgi.getCookie('CGI_COOKIE_TEST', 'Not Set')}\n")
cgi.sendBody("You can also access the request body:\n")
cgi.sendBody(f"Request Body: {cgi.getBody()}\n")
cgi.sendBody("This is the end of the CGI script.\n")