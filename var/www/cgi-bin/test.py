import time
import sys

try:
    print("Hello world from Python CGI script!")
    sys.stdout.flush()
    time.sleep(10)
    print("This is a test CGI script.")
    sys.stdout.flush()
except BrokenPipeError:
    pass  # Prevent error on server if client disconnects