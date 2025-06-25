#!/usr/bin/env python3

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import random
import cgilib
import json
import string

cgi: cgilib.CGIClient = cgilib.CGIClient()

cgi.setHeader("Content-Type", "application/json; charset=utf-8")
cgi.setStatus(cgilib.HttpStatusCode.OK)

def random_email(username):
    domains = ["example.com", "mail.com", "test.org", "demo.net"]
    return f"{username or 'user'}{random.randint(1,9999)}@{random.choice(domains)}"

def random_country():
    countries = ["Netherlands", "USA", "Germany", "France", "Japan", "Brazil", "India"]
    return random.choice(countries)

def random_phone():
    return "+{} {}-{}-{}".format(
        random.randint(1, 99),
        random.randint(100, 999),
        random.randint(100, 999),
        random.randint(1000, 9999)
    )

def random_iq():
    return random.randint(80, 160)

username = cgi.getPathParameter(0, None)

cgi.sendBody(json.dumps({
    "user": username,
    "email": random_email(username),
    "country": random_country(),
    "phone": random_phone(),
    "iq": random_iq()
}))