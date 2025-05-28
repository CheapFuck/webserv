#include "config/consts.hpp"
#include "response.hpp"
#include "methods.hpp"
#include "print.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

/// @brief Helper function to get the file extension from a given path.
/// @param path The file path from which to extract the extension.
/// @return The file extension in lowercase, or an empty string if no extension is found.
static std::string get_file_extension(const std::string& path) {
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) return "";
    std::string ext = path.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

/// @brief Get the MIME type for a given file path based on its extension.
/// @param path The file path for which to determine the MIME type.
/// @return The MIME type as a string. If the extension is not recognized,
/// returns "application/octet-stream".
std::string getMimeType(const std::string& path) {
    static const std::unordered_map<std::string, std::string> mime_types = {
        {"html", "text/html"},
        {"htm", "text/html"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"xml", "application/xml"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"bmp", "image/bmp"},
        {"webp", "image/webp"},
        {"svg", "image/svg+xml"},
        {"ico", "image/x-icon"},
        {"txt", "text/plain"},
        {"pdf", "application/pdf"},
        {"zip", "application/zip"},
        {"tar", "application/x-tar"},
        {"gz", "application/gzip"},
        {"mp3", "audio/mpeg"},
        {"mp4", "video/mp4"},
        {"ogg", "application/ogg"},
        {"wav", "audio/wav"},
        {"avi", "video/x-msvideo"},
        {"mov", "video/quicktime"},
        {"woff", "font/woff"},
        {"woff2", "font/woff2"},
        {"ttf", "font/ttf"},
        {"otf", "font/otf"},
        {"eot", "application/vnd.ms-fontobject"},
        {"wasm", "application/wasm"},
    };

    std::string ext = get_file_extension(path);
    auto it = mime_types.find(ext);
    if (it == mime_types.end())
        return ("application/octet-stream");
    return (it->second);
}

/// @brief Attempts to create a Response object from a file.
/// @details This function will set the body of the response; but won't touch the status code.
/// @param filepath The path to the file from which to create the response.
/// @param response The Response object to populate with the file's content.
/// @return True if the response was successfully created from the file, false otherwise.
bool tryCreateResponseFromFile(const Path &filepath, Response &response) {
    DEBUG("Trying to create response from file: " << filepath.str());
    std::ifstream file(filepath.str(), std::ios::binary);

    if (!file) {
        DEBUG("File not found: " << filepath.str());
        return (false);
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    file.close();

    response.headers.replace(HeaderKey::ContentType, getMimeType(filepath.str()));
    response.setBody(ss.str());
    return (true);
}
