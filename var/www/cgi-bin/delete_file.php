#!/usr/bin/php-cgi
<?php
if ($_SERVER['REQUEST_METHOD'] === 'DELETE') {
    parse_str(file_get_contents("php://input"), $data);
    header("Content-Type: application/json");

    $filename = $data['file'] ?? '';
    $filePath = __DIR__ . '/../uploads/' . basename($filename);

    if (!file_exists($filePath)) {
        http_response_code(404);
        echo json_encode(["error" => "File not found.", "path" => $filePath]);
        exit;
    }

    if (unlink($filePath)) {
        http_response_code(200);
        echo json_encode(["message" => "File deleted successfully."]);
    } else {
        http_response_code(500);
        echo json_encode(["error" => "Failed to delete file."]);
    }
} else {
    http_response_code(405);
    echo json_encode(["error" => "Method Not Allowed"]);
}
?>
