#pragma once

#include <string>
#include <iostream>
enum Method {
	UNKNOWN_METHOD = 0,
	GET = 1 << 0,
	POST = 1 << 1,
	DELETE = 1 << 2,
	PUT = 1 << 3,
	HEAD = 1 << 4,
	OPTIONS = 1 << 5,
};

#define ALL_METHODS (GET | POST | DELETE | PUT | HEAD | OPTIONS)

Method operator|(Method lhs, Method rhs);
Method stringToMethod(const std::string &str);
std::string methodToStr(const Method &method);

std::ostream &operator<<(std::ostream &os, const Method &method);

enum class HttpStatusCode {
    // 1xx
    Continue = 100,
    SwitchingProtocols = 101,
    Processing = 102,
    EarlyHints = 103,

    // 2xx
    OK = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    MultiStatus = 207,
    AlreadyReported = 208,
    IMUsed = 226,

    // 3xx
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,

    // 4xx
    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    PayloadTooLarge = 413,
    URITooLong = 414,
    UnsupportedMediaType = 415,
    RangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    ImATeapot = 418,
    MisdirectedRequest = 421,
    UnprocessableEntity = 422,
    Locked = 423,
    FailedDependency = 424,
    TooEarly = 425,
    UpgradeRequired = 426,
    PreconditionRequired = 428,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    UnavailableForLegalReasons = 451,

    // 5xx
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505,
    VariantAlsoNegotiates = 506,
    InsufficientStorage = 507,
    LoopDetected = 508,
    NotExtended = 510,
    NetworkAuthenticationRequired = 511
};

std::string getStatusCodeAsStr(HttpStatusCode code);
std::ostream &operator<<(std::ostream &os, HttpStatusCode code);

const std::string default_err_body(const char *error_text, HttpStatusCode error_code);

const std::string getDefaultBodyForCode(HttpStatusCode code);

enum class HeaderKey {
    ContentType,
    ContentLength,
    Host,
    UserAgent,
    Accept,
    AcceptEncoding,
    AcceptLanguage,
    Connection,
    Cookie,
    SetCookie,
    Authorization,
    Date,
    Location,
    Referer,
    Status,
};

std::string headerKeyToString(HeaderKey key);
std::ostream &operator<<(std::ostream &os, HeaderKey key);