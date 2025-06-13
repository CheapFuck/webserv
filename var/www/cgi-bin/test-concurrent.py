#!/usr/bin/env python3
import requests
import threading
import time

def make_request(url, i):
    print(f"Starting request {i} to {url}")
    start_time = time.time()
    response = requests.get(url)
    end_time = time.time()
    print(f"Request {i} completed in {end_time - start_time:.2f} seconds with status {response.status_code}")
    print(f"Response: {response.text[:50]}...")

# Make 10 concurrent requests
threads = []
for i in range(10):
    t = threading.Thread(target=make_request, args=("http://localhost:8080/cgi-bin/test.py", i))
    threads.append(t)
    t.start()

# Wait for all threads to complete
for t in threads:
    t.join()

print("All requests completed")