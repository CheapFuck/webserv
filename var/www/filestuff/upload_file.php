#!/usr/bin/php-cgi
<?php
    ini_set('display_errors', 'stderr');
    error_reporting(E_ALL);
    header("Content-Type: application/json");

    if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
        header('Status: 405 Method Not Allowed');
        echo json_encode(['error' => 'Method Not Allowed']);
        exit;
    }

    if (!isset($_FILES['file']) || $_FILES['file']['error'] !== UPLOAD_ERR_OK) {
        header("Status: 400 Bad Request");
        echo json_encode(['error' => 'File upload failed.']);
        exit;
    }

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

    $originalName = basename($_FILES['file']['name']);
    $destination = rtrim($directory, '/') . '/' . $originalName;

    if (move_uploaded_file($_FILES['file']['tmp_name'], $destination)) {
        header("Status: 201 Created");
        echo json_encode(['message' => 'File uploaded successfully', 'file' => $originalName]);
    } else {
        header("Status: 500 Internal Server Error");
        echo json_encode(['error' => 'Failed to move uploaded file']);
    }
?>
