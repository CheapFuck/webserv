#!/usr/bin/php-cgi
<?php
$directory = '../uploads/';

if (!is_dir($directory)) {
    http_response_code(500);
    echo json_encode(['error' => 'Directory not found']);
    exit;
}
header("Status: 200");

$files = array_diff(scandir($directory), array('..', '.'));
echo json_encode(array_values($files));
?>
