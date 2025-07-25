#!/usr/bin/php-cgi

<html>
    <link rel="stylesheet" type="text/css" href="<?php echo getenv('HTTP_CSS_FILE'); ?>">
</html>

<?php
    require 'webservsession.php';
    startWebservSession();

    header("Content-Type: text/html");
    header("Status: 200 OK");

    $_SESSION['pph_message'] = "This is a message from another.php";

    echo "Hello from a PHP CGI script!\n\nFrom now on, there will be a session variable set with the name 'pph_message'.\n";
?>