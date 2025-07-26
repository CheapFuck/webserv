#!/usr/bin/php-cgi
<?php
    function nl2br_custom($string) {
        return str_replace(array("\r\n", "\n", "\r"), '<br>', $string);
    }

    require 'webservsession.php';
    startWebservSession();

    header("Content-Type: text/html");
    header("Status: 200 OK");

    $_SESSION['php_message'] = "This is a message from another.php";

    echo "Hello from a PHP CGI script!<br><br>From now on, there will be a session variable set with the name 'php_message'.<br><br>";
    echo "Current session data:<br><br>";
    echo nl2br_custom(json_encode($_SESSION, JSON_PRETTY_PRINT));
?>`