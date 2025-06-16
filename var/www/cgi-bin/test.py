import time

output = []
output.append("Hello world from Python CGI script!")
time.sleep(10)
output.append("This is a test CGI script.")

print("\n".join(output))