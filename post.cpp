#include "config/rules/rules.hpp"
#include "response.hpp"
#include "methods.hpp"
#include "print.hpp"

#include <filesystem>
#include <fstream>

static std::string extractBoundary(const std::string& contentType) {
    size_t pos = contentType.find("boundary=");
    if (pos != std::string::npos) {
        size_t endPos = contentType.find(',', pos);
        if (endPos != std::string::npos) {
            return "--" + contentType.substr(pos + 9, endPos - pos - 9);
        } else
            return "--" + contentType.substr(pos + 9);
    }
    return "";
}

static std::string extractBetween(const std::string& str, const std::string& start, const std::string& end) {
    size_t s = str.find(start);
    if (s == std::string::npos) return "";
    s += start.length();
    size_t e = str.find(end, s);
    return (e == std::string::npos) ? "" : str.substr(s, e - s);
}

static void parseMultipartFormData(const std::string& body, const std::string& boundary,
    std::map<std::string, std::string>& formFields,
    std::vector<PostMethod::FilePart>& files) {
    size_t pos = 0, next;

    DEBUG_IF_NOT(body.size(), "No body in request");
    DEBUG("Parsing multipart form data with boundary: " << boundary);

    while ((next = body.find(boundary, pos)) != std::string::npos) {
        DEBUG("Found boundary at position: " << next);
        size_t headerEnd = body.find("\r\n\r\n", pos);
        if (headerEnd == std::string::npos) break;

        std::string headers = body.substr(pos, headerEnd - pos);
        size_t contentStart = headerEnd + 4;
        size_t contentEnd = body.find(boundary, contentStart) - 2;

        std::string content = body.substr(contentStart, contentEnd - contentStart);
        PostMethod::FilePart part;

        if (headers.find("filename=\"") != std::string::npos) {
            part.name = extractBetween(headers, "name=\"", "\"");
            part.filename = extractBetween(headers, "filename=\"", "\"");
            part.content = content;
            files.push_back(part);
        } else {
            std::string name = extractBetween(headers, "name=\"", "\"");
            formFields[name] = content;
        }

        pos = contentEnd + boundary.length() + 2;
    }
}

static void tryEnsureUploadDir(const std::string& full_path)
{
    std::error_code errcode;
    std::filesystem::create_directories(full_path, errcode);
    ERROR_IF(errcode, "Failed to create directories: " << errcode.message());
}

static Response &uploadFile(
    const Request &request,
    Response &response,
    const std::string &content_type,
    const LocationRule &route
) {
    std::map<std::string, std::string> formFields;
    std::vector<PostMethod::FilePart> files;

    parseMultipartFormData(request.getBody(), extractBoundary(content_type), formFields, files);
    tryEnsureUploadDir(route.upload_dir.get().str());

    for (const PostMethod::FilePart &file : files) {
        Path uploadDir(route.upload_dir.get());
        uploadDir.append(file.filename);
        if (!uploadDir.isValid()) {
            response.setStatusCode(HttpStatusCode::BadRequest);
            return (response);
        }
        std::ofstream outFile(uploadDir.str(), std::ios::binary);
        if (outFile) {
            outFile.write(file.content.c_str(), file.content.length());
            outFile.close();
        } else {
            response.setStatusCode(HttpStatusCode::InternalServerError);
            return (response);
        }
    }

    if (files.empty()) {
        response.setStatusCode(HttpStatusCode::BadRequest);
        response.setBody("No files uploaded");
        return (response);
    }

    response.setStatusCode(HttpStatusCode::OK);
    response.headers.replace(HeaderKey::ContentType, "text/plain");
    response.setBody("File(s) uploaded successfully");
    return (response);   
}

Response &PostMethod::processRequest(
    const Request &request,
    Response &response,
    const ServerConfig &config,
    const LocationRule &route
) {
    (void)config;
    DEBUG("Processing POST request for path: " << request.metadata.getRawUrl());

    std::string content_type = request.headers.getHeader(HeaderKey::ContentType, "");
    if (content_type.empty()) {
        response.setStatusCode(HttpStatusCode::UnsupportedMediaType);
        DEBUG("Missing Content-Type header");
        return response;
    }

    if (content_type.find("multipart/form-data") == std::string::npos) {
        response.setStatusCode(HttpStatusCode::UnsupportedMediaType);
        DEBUG("Unsupported Content-Type: " << content_type);
        return response;
    }

    return (uploadFile(request, response, content_type, route));
}
