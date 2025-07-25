#include "config/types/consts.hpp"
#include "response.hpp"
#include "methods.hpp"
#include "print.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

/// @brief Attempts to create a Response object from a file.
/// @details This function will set the body of the response; but won't touch the status code.
/// @param filepath The path to the file from which to create the response.
/// @param response The Response object to populate with the file's content.
/// @return True if the response was successfully created from the file, false otherwise.
// bool tryCreateResponseFromFile(const Path &filepath, Response &response) {
//     DEBUG("Trying to create response from file: " << filepath.str());
//     std::ifstream file(filepath.str(), std::ios::binary);

//     if (!file) {
//         DEBUG("File not here22 found: " << filepath.str());
//         return (false);
//     }

//     std::ostringstream ss;
//     ss << file.rdbuf();
//     file.close();

//     response.headers.replace(HeaderKey::ContentType, getMimeType(filepath.str()));
//     response.setBody(ss.str());
//     return (true);
// }
