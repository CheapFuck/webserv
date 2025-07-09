#include "customTypes.hpp"
#include "../../print.hpp"
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
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    if (lowerStr == "get") return GET;
	if (lowerStr == "post") return POST;
	if (lowerStr == "delete") return DELETE;
	if (lowerStr == "put") return PUT;
	if (lowerStr == "head") return HEAD;
	if (lowerStr == "options") return OPTIONS;
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

HttpStatusCode fromStatusCode(StatusCode code) {
    return static_cast<HttpStatusCode>(code.value);
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
