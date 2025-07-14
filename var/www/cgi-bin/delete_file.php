#!/usr/bin/php-cgi
<?php
if ($_SERVER['REQUEST_METHOD'] === 'DELETE') {
    parse_str(file_get_contents("php://input"), $data);
    header("Content-Type: application/json");

    $filename = $data['file'] ?? '';
    $filePath = __DIR__ . '/../uploads/' . basename($filename);

    if (!file_exists($filePath)) {
        header('Status: 404 Not Found');
        echo json_encode(["error" => "File not found.", "path" => $filePath]);
        exit;
    }

    if (unlink($filePath)) {
        header('Status: 200 OK');
        echo json_encode(["message" => "File deleted successfully."]);
    } else {
        header('Status: 500 Internal Server Error');
        echo json_encode(["error" => "Failed to delete file."]);
    }
} else {
    header('Status: 405 Method Not Allowed');
    echo json_encode(["error" => "Method Not Allowed"]);
}
?>
