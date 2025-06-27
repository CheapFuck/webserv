#!/usr/bin/php-cgi
<?php
if ($_SERVER['REQUEST_METHOD'] === 'DELETE') {
    parse_str(file_get_contents("php://input"), $data);
    header("Content-Type: application/json");
    $filename = $data['file'] ?? '';
file_put_contents('/tmp/debug_input.txt', file_get_contents('php://input'));
    $filePath = __DIR__ . '/../uploads/' . basename($filename);

    if (!file_exists($filePath)) {
        header("Status: 404");
        echo "File not found.";
        echo $filePath;
        exit;
    }

    if (unlink($filePath)) {
        header("Status: 200");
        echo "File deleted successfully.";
    } else {
        header("Status: 500");
        echo "Failed to delete file.";
    }
} else {
    header("Status: 405");
    echo "Method Not Allowed";
}
?>
