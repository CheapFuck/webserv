#include "../print.hpp"
#include "consts.hpp"

#include <type_traits>
#include <string>

Method operator|(Method lhs, Method rhs) {
	return static_cast<Method>(
		static_cast<std::underlying_type<Method>::type>(lhs) |
		static_cast<std::underlying_type<Method>::type>(rhs)
	);
}

Method stringToMethod(const std::string &str) {
	if (str == "GET") return GET;
	if (str == "POST") return POST;
	if (str == "DELETE") return DELETE;
	if (str == "PUT") return PUT;
	if (str == "HEAD") return HEAD;
	if (str == "OPTIONS") return OPTIONS;
	return UNKNOWN_METHOD;
}

std::string methodToStr(const Method &method) {
	std::string str;

	if (method & GET) str += "GET|";
	if (method & POST) str += "POST|";
	if (method & DELETE) str += "DELETE|";
	if (method & PUT) str += "PUT|";
	if (method & HEAD) str += "HEAD|";
	if (method & OPTIONS) str += "OPTIONS|";
	if (!str.empty()) str.pop_back();
	if (str.empty()) return "UNKNOWN";

	return str;
}

std::ostream& operator<<(std::ostream& os, const Method& method) {
	os << methodToStr(method);
	return os;
}

std::string getStatusCodeAsStr(HttpStatusCode code) {
    switch (code) {
        // 1xx
        case HttpStatusCode::Continue: return "Continue";
        case HttpStatusCode::SwitchingProtocols: return "Switching Protocols";
        case HttpStatusCode::Processing: return "Processing";
        case HttpStatusCode::EarlyHints: return "Early Hints";

        // 2xx
        case HttpStatusCode::OK: return "OK";
        case HttpStatusCode::Created: return "Created";
        case HttpStatusCode::Accepted: return "Accepted";
        case HttpStatusCode::NonAuthoritativeInformation: return "Non-Authoritative Information";
        case HttpStatusCode::NoContent: return "No Content";
        case HttpStatusCode::ResetContent: return "Reset Content";
        case HttpStatusCode::PartialContent: return "Partial Content";
        case HttpStatusCode::MultiStatus: return "Multi-Status";
        case HttpStatusCode::AlreadyReported: return "Already Reported";
        case HttpStatusCode::IMUsed: return "IM Used";

        // 3xx
        case HttpStatusCode::MultipleChoices: return "Multiple Choices";
        case HttpStatusCode::MovedPermanently: return "Moved Permanently";
        case HttpStatusCode::Found: return "Found";
        case HttpStatusCode::SeeOther: return "See Other";
        case HttpStatusCode::NotModified: return "Not Modified";
        case HttpStatusCode::UseProxy: return "Use Proxy";
        case HttpStatusCode::TemporaryRedirect: return "Temporary Redirect";
        case HttpStatusCode::PermanentRedirect: return "Permanent Redirect";

        // 4xx
        case HttpStatusCode::BadRequest: return "Bad Request";
        case HttpStatusCode::Unauthorized: return "Unauthorized";
        case HttpStatusCode::PaymentRequired: return "Payment Required";
        case HttpStatusCode::Forbidden: return "Forbidden";
        case HttpStatusCode::NotFound: return "Not Found";
        case HttpStatusCode::MethodNotAllowed: return "Method Not Allowed";
        case HttpStatusCode::NotAcceptable: return "Not Acceptable";
        case HttpStatusCode::ProxyAuthenticationRequired: return "Proxy Authentication Required";
        case HttpStatusCode::RequestTimeout: return "Request Timeout";
        case HttpStatusCode::Conflict: return "Conflict";
        case HttpStatusCode::Gone: return "Gone";
        case HttpStatusCode::LengthRequired: return "Length Required";
        case HttpStatusCode::PreconditionFailed: return "Precondition Failed";
        case HttpStatusCode::PayloadTooLarge: return "Payload Too Large";
        case HttpStatusCode::URITooLong: return "URI Too Long";
        case HttpStatusCode::UnsupportedMediaType: return "Unsupported Media Type";
        case HttpStatusCode::RangeNotSatisfiable: return "Range Not Satisfiable";
        case HttpStatusCode::ExpectationFailed: return "Expectation Failed";
        case HttpStatusCode::ImATeapot: return "I'm a teapot";
        case HttpStatusCode::MisdirectedRequest: return "Misdirected Request";
        case HttpStatusCode::UnprocessableEntity: return "Unprocessable Entity";
        case HttpStatusCode::Locked: return "Locked";
        case HttpStatusCode::FailedDependency: return "Failed Dependency";
        case HttpStatusCode::TooEarly: return "Too Early";
        case HttpStatusCode::UpgradeRequired: return "Upgrade Required";
        case HttpStatusCode::PreconditionRequired: return "Precondition Required";
        case HttpStatusCode::TooManyRequests: return "Too Many Requests";
        case HttpStatusCode::RequestHeaderFieldsTooLarge: return "Request Header Fields Too Large";
        case HttpStatusCode::UnavailableForLegalReasons: return "Unavailable For Legal Reasons";

        // 5xx
        case HttpStatusCode::InternalServerError: return "Internal Server Error";
        case HttpStatusCode::NotImplemented: return "Not Implemented";
        case HttpStatusCode::BadGateway: return "Bad Gateway";
        case HttpStatusCode::ServiceUnavailable: return "Service Unavailable";
        case HttpStatusCode::GatewayTimeout: return "Gateway Timeout";
        case HttpStatusCode::HTTPVersionNotSupported: return "HTTP Version Not Supported";
        case HttpStatusCode::VariantAlsoNegotiates: return "Variant Also Negotiates";
        case HttpStatusCode::InsufficientStorage: return "Insufficient Storage";
        case HttpStatusCode::LoopDetected: return "Loop Detected";
        case HttpStatusCode::NotExtended: return "Not Extended";
        case HttpStatusCode::NetworkAuthenticationRequired: return "Network Authentication Required";

        default:
            ERROR("Unknown HTTP Status Code: " << static_cast<int>(code));
            return "Unknown Status Code";
    }
}

std::ostream& operator<<(std::ostream& os, HttpStatusCode code) {
	os << getStatusCodeAsStr(code);
	return os;
}

std::string headerKeyToString(HeaderKey key) {
    switch (key) {
        case HeaderKey::ContentType: return "Content-Type";
        case HeaderKey::ContentLength: return "Content-Length";
        case HeaderKey::Host: return "Host";
        case HeaderKey::UserAgent: return "User-Agent";
        case HeaderKey::Accept: return "Accept";
        case HeaderKey::AcceptEncoding: return "Accept-Encoding";
        case HeaderKey::AcceptLanguage: return "Accept-Language";
        case HeaderKey::Connection: return "Connection";
        case HeaderKey::Cookie: return "Cookie";
        case HeaderKey::SetCookie: return "Set-Cookie";
        case HeaderKey::Authorization: return "Authorization";
        case HeaderKey::Date: return "Date";
        case HeaderKey::Location: return "Location";
        case HeaderKey::Referer: return "Referer";
        case HeaderKey::Status: return "Status";

        default:
            ERROR("Unknown Header Key: " << static_cast<int>(key));
            return "Unknown Header Key";
    }
}

std::ostream& operator<<(std::ostream& os, HeaderKey key) {
    os << headerKeyToString(key);
    return os;
}

const std::string default_err_body(const char *error_text, HttpStatusCode error_code)
{
	std::string todisplay = std::to_string(static_cast<int>(error_code)) + " - " + error_text;
	std::string s = R"(<!DOCTYPE html>
	<html lang="en">
	<head>
	<meta charset="UTF-8">
	<title>[TITLEPLACE]</title>
	<style>
		html, body {
		height: 100%;
		margin: 0px;
		}
		body {
		display: flex;
		justify-content: center;
		border-style: inset;
		border-width: 9px;
		align-items: center;
		font-size: 2em;
		background-color: #f0f0f0;
		}
	</style>
	</head>
	<body>
	Error [ERRORTEXTPLACE]
	</body>
	</html>)";
	s.replace(s.find("[ERRORTEXTPLACE]"), 16, todisplay);
	s.replace(s.find("[TITLEPLACE]"), 12, todisplay);

	return (s);
}

const std::string getDefaultBodyForCode(HttpStatusCode code)
{
    switch(code)
    {
        case HttpStatusCode::Continue: return default_err_body("Continue", code);
        case HttpStatusCode::SwitchingProtocols: return default_err_body("SwitchingProtocols", code);
        case HttpStatusCode::Processing: return default_err_body("Processing", code);
        case HttpStatusCode::EarlyHints: return default_err_body("EarlyHints", code);

        // 2xx
        case HttpStatusCode::OK: return default_err_body("OK", code);
        case HttpStatusCode::Created: return default_err_body("Created", code);
        case HttpStatusCode::Accepted: return default_err_body("Accepted", code);
        case HttpStatusCode::NonAuthoritativeInformation: return default_err_body("NonAuthoritativeInformation", code);
        case HttpStatusCode::NoContent: return default_err_body("NoContent", code);
        case HttpStatusCode::ResetContent: return default_err_body("ResetContent", code);
        case HttpStatusCode::PartialContent: return default_err_body("PartialContent", code);
        case HttpStatusCode::MultiStatus: return default_err_body("MultiStatus", code);
        case HttpStatusCode::AlreadyReported: return default_err_body("AlreadyReported", code);
        case HttpStatusCode::IMUsed: return default_err_body("IMUsed", code);

        // 3xx
        case HttpStatusCode::MultipleChoices: return default_err_body("MultipleChoices", code);
        case HttpStatusCode::MovedPermanently: return default_err_body("MovedPermanently", code);
        case HttpStatusCode::Found: return default_err_body("Found", code);
        case HttpStatusCode::SeeOther: return default_err_body("SeeOther", code);
        case HttpStatusCode::NotModified: return default_err_body("NotModified", code);
        case HttpStatusCode::UseProxy: return default_err_body("UseProxy", code);
        case HttpStatusCode::TemporaryRedirect: return default_err_body("TemporaryRedirect", code);
        case HttpStatusCode::PermanentRedirect: return default_err_body("PermanentRedirect", code);

        // 4xx
        case HttpStatusCode::BadRequest: return default_err_body("BadRequest", code);
        case HttpStatusCode::Unauthorized: return default_err_body("Unauthorized", code);
        case HttpStatusCode::PaymentRequired: return default_err_body("PaymentRequired", code);
        case HttpStatusCode::Forbidden: return default_err_body("Forbidden", code);
        case HttpStatusCode::NotFound: return default_err_body("NotFound", code);
        case HttpStatusCode::MethodNotAllowed: return default_err_body("MethodNotAllowed", code);
        case HttpStatusCode::NotAcceptable: return default_err_body("NotAcceptable", code);
        case HttpStatusCode::ProxyAuthenticationRequired: return default_err_body("ProxyAuthenticationRequired", code);
        case HttpStatusCode::RequestTimeout: return default_err_body("RequestTimeout", code);
        case HttpStatusCode::Conflict: return default_err_body("Conflict", code);
        case HttpStatusCode::Gone: return default_err_body("Gone", code);
        case HttpStatusCode::LengthRequired: return default_err_body("LengthRequired", code);
        case HttpStatusCode::PreconditionFailed: return default_err_body("PreconditionFailed", code);
        case HttpStatusCode::PayloadTooLarge: return default_err_body("PayloadTooLarge", code);
        case HttpStatusCode::URITooLong: return default_err_body("URITooLong", code);
        case HttpStatusCode::UnsupportedMediaType: return default_err_body("UnsupportedMediaType", code);
        case HttpStatusCode::RangeNotSatisfiable: return default_err_body("RangeNotSatisfiable", code);
        case HttpStatusCode::ExpectationFailed: return default_err_body("ExpectationFailed", code);
        case HttpStatusCode::ImATeapot: return default_err_body("ImATeapot", code);
        case HttpStatusCode::MisdirectedRequest: return default_err_body("MisdirectedRequest", code);
        case HttpStatusCode::UnprocessableEntity: return default_err_body("UnprocessableEntity", code);
        case HttpStatusCode::Locked: return default_err_body("Locked", code);
        case HttpStatusCode::FailedDependency: return default_err_body("FailedDependency", code);
        case HttpStatusCode::TooEarly: return default_err_body("TooEarly", code);
        case HttpStatusCode::UpgradeRequired: return default_err_body("UpgradeRequired", code);
        case HttpStatusCode::PreconditionRequired: return default_err_body("PreconditionRequired", code);
        case HttpStatusCode::TooManyRequests: return default_err_body("TooManyRequests", code);
        case HttpStatusCode::RequestHeaderFieldsTooLarge: return default_err_body("RequestHeaderFieldsTooLarge", code);
        case HttpStatusCode::UnavailableForLegalReasons: return default_err_body("UnavailableForLegalReasons", code);

        // 5xx
        case HttpStatusCode::InternalServerError: return default_err_body("InternalServerError", code);
        case HttpStatusCode::NotImplemented: return default_err_body("NotImplemented", code);
        case HttpStatusCode::BadGateway: return default_err_body("BadGateway", code);
        case HttpStatusCode::ServiceUnavailable: return default_err_body("ServiceUnavailable", code);
        case HttpStatusCode::GatewayTimeout: return default_err_body("GatewayTimeout", code);
        case HttpStatusCode::HTTPVersionNotSupported: return default_err_body("HTTPVersionNotSupported", code);
        case HttpStatusCode::VariantAlsoNegotiates: return default_err_body("VariantAlsoNegotiates", code);
        case HttpStatusCode::InsufficientStorage: return default_err_body("InsufficientStorage", code);
        case HttpStatusCode::LoopDetected: return default_err_body("LoopDetected", code);
        case HttpStatusCode::NotExtended: return default_err_body("NotExtended", code);
        case HttpStatusCode::NetworkAuthenticationRequired: return default_err_body("NetworkAuthenticationRequired", code);
        default: return default_err_body("How did you get here?!", code);
    }
}