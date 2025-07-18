#!/bin/bash

# Define the ASCII cat
read -r -d '' CAT <<'EOF'
 /\_/\  
( o.o ) 
 > ^ <
 /\_/\  
( o.o ) 
 > ^ <
 /\_/\  
( o.o ) 
 > ^ <
 /\_/\  
( o.o ) 
 > ^ <
EOF

send_chunked_request() {
  {
    while IFS= read -r line; do
      len=$(printf "%x" ${#line})
      printf "%s\r\n%s\r\n" "$len" "$line"
      sleep 1
    done <<< "$CAT"
    printf "0\r\n\r\n"
  } | curl -v -X POST http://localhost:8080/upload \
       -H "Transfer-Encoding: chunked" \
       -H "Content-Type: text/plain" \
       --data-binary @-
}

send_chunked_request
