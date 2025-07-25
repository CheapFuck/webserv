#!/usr/bin/php-cgi
<?php
    $directory = getenv('WEBSERV_UPLOAD_STORE');
    if (!$directory) {
        header("Status: 500 Internal Server Error");
        echo json_encode(['error' => 'Invalid environment variables set']);
        exit;
    }

    if (!is_dir($directory)) {
        header("Status: 404 Not Found");
        echo json_encode(['error' => 'Directory not found']);
        exit;
    }

    header("Status: 200");
    $files = array_diff(scandir($directory), array('..', '.'));
    echo json_encode(array_values($files));
?>
