#!/usr/bin/php-cgi
<?php
    header("Content-Type: application/json");

    if ($_SERVER['REQUEST_METHOD'] !== 'DELETE') {
        header('Status: 405 Method Not Allowed');
        echo json_encode(['error' => 'Method Not Allowed']);
        exit;
    }

    $directory = getenv('WEBSERV_UPLOAD_STORE');
    if (!$directory) {
        header('Status: 500 Internal Server Error');
        echo json_encode(['error' => 'Invalid environment variables set']);
        exit;
    }

    parse_str(file_get_contents("php://input"), $data);

    $filename = $data['file'] ?? '';
    $filePath = $directory . '/' . basename($filename);

    if (!file_exists($filePath)) {
        header('Status: 404 Not Found');
        echo json_encode(["error" => "File not found.", "path" => $filePath, "sapi" => php_sapi_name()]);
        exit;
    }

    if (unlink($filePath)) {
        header('Status: 200 OK');
        echo json_encode(["message" => "File deleted successfully.", "path" => $filePath]);
    } else {
        header('Status: 500 Internal Server Error');
        echo json_encode(["error" => "Failed to delete file.", "path" => $filePath]);
    }
?>
