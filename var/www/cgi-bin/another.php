#!/usr/bin/php-cgi

<?php
    header("Content-Type: text/html");
    http_response_code(201);

    echo "Hello from another PHP CGI script!\n";
?>