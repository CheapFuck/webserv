<?php
    /// This script initializes a session for a web server CGI application.
    /// Access the session by using the global variable $webservSession.
    function startWebservSession() {
        $sessionFile = getenv('HTTP_SESSION_FILE');
        if (!$sessionFile)
            throw new Exception("Session file not set in environment variable HTTP_SESSION_FILE");

        $sessionData = '';
        $sessionJson = [];

        if (file_exists($sessionFile)) {
            $sessionData = file_get_contents($sessionFile);
            $sessionJson = json_decode($sessionData, true);
        }

        $GLOBALS['webservSession'] = $sessionJson;

        register_shutdown_function(function() use ($sessionFile) {
            echo "Session data saved.\n";
            echo "Session content: " . json_encode($GLOBALS['webservSession']) . "\n";
            if (isset($GLOBALS['webservSession'])) {
                file_put_contents($sessionFile, json_encode($GLOBALS['webservSession']));
            }
        });
    }
?>