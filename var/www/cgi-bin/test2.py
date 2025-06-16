import sys
import time
import signal

def handle_sigpipe(signum, frame):
    print("[DEBUG] SIGPIPE received, exiting.", file=sys.stderr, flush=True)
    sys.exit(0)

# Handle SIGPIPE gracefully
signal.signal(signal.SIGPIPE, handle_sigpipe)

try:
    print("[DEBUG] CGI script started", file=sys.stderr, flush=True)
    print("Content-type: text/html\n", flush=True)
    print("[DEBUG] Printed Content-type header", file=sys.stderr, flush=True)
    print("Hello world from Python CGI script!", flush=True)
    print("[DEBUG] Printed hello message", file=sys.stderr, flush=True)
    time.sleep(10)
    print("This is a test CGI TWEE script.", flush=True)
    print("[DEBUG] Printed final message", file=sys.stderr, flush=True)
except BrokenPipeError:
    print("[DEBUG] BrokenPipeError caught, exiting.", file=sys.stderr, flush=True)
    sys.exit(0)
except Exception as e:
    print(f"<pre>CGI Exception: {e}</pre>", flush=True)
    print(f"[DEBUG] Exception: {e}", file=sys.stderr, flush=True)
    sys.exit(1)