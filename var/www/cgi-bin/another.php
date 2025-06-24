#!/usr/bin/php-cgi

<?php
    require 'webservsession.php';
    startWebservSession();

    header("Content-Type: text/html");
    header("Status: 200 OK");

    if (isset($sessionJson['message'])) {
        echo "Message from session: " . htmlspecialchars($sessionJson['message']) . "\n";
    } else {
        echo "No message in session.\n";
    }
    $webservSession['message'] = "This is a new message from another.php script v2.";

    echo "Hello from another PHP CGI script!\n";
?>