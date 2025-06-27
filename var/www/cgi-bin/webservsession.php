<?php
    function startWebservSession() {
        $sessionFile = getenv('HTTP_SESSION_FILE');
        if (!$sessionFile)
            throw new Exception("Session file not set in environment variable HTTP_SESSION_FILE");

        if (file_exists($sessionFile)) {
            $sessionData = file_get_contents($sessionFile);
            $sessionJson = json_decode($sessionData, true);
            $GLOBALS['_SESSION'] = $sessionJson;
        } else {
            $GLOBALS['_SESSION'] = [];
        }

        register_shutdown_function(function() use ($sessionFile) {
            file_put_contents($sessionFile, json_encode($GLOBALS['_SESSION']));
        });
    }
?>