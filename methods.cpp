#include "config/consts.hpp"
#include "response.hpp"
#include "methods.hpp"
#include "print.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

static std::string get_file_extension(const std::string& path) {
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) return "";
    std::string ext = path.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

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
    response.setStatusCode(HttpStatusCode::OK);
    response.setBody(ss.str());
    return (true);
}
