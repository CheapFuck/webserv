#!/usr/bin/php-cgi

<html>
    <link rel="stylesheet" type="text/css" href="<?php echo getenv('HTTP_CSS_FILE'); ?>">
</html>

<?php
    require 'webservsession.php';
    startWebservSession();

    header("Content-Type: text/html");
    header("Status: 200 OK");

    $_SESSION['message'] = "This is a new message from another.php script v2.";

    echo "Hello from another PHP CGI script!\n";
?>