#!/usr/bin/php
<?php
// Simple PHP CGI test script
echo "Content-Type: text/html\r\n";
echo "\r\n";
echo "<html><head><title>PHP CGI Test</title></head><body>";
echo "<h1>PHP CGI Test</h1>";
echo "<p>Request Method: " . $_SERVER['REQUEST_METHOD'] . "</p>";
echo "<p>Script Name: " . $_SERVER['SCRIPT_NAME'] . "</p>";
echo "<p>Query String: " . $_SERVER['QUERY_STRING'] . "</p>";
echo "<p>Server Name: " . $_SERVER['SERVER_NAME'] . "</p>";
echo "<p>Server Port: " . $_SERVER['SERVER_PORT'] . "</p>";

if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    echo "<h2>POST Data:</h2>";
    echo "<pre>";
    $input = file_get_contents('php://stdin');
    echo htmlspecialchars($input);
    echo "</pre>";
}

echo "<h2>Environment Variables:</h2>";
echo "<pre>";
foreach ($_SERVER as $key => $value) {
    echo htmlspecialchars($key . " = " . $value) . "\n";
}
echo "</pre>";

echo "</body></html>";
?>
