#pragma once

#include "config/types/customTypes.hpp"

#include <string>

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

HttpStatusCode fromStatusCode(StatusCode code);
std::string getStatusCodeAsStr(HttpStatusCode code);
std::ostream &operator<<(std::ostream &os, HttpStatusCode code);

constexpr const char* getDefaultBodyForCode(HttpStatusCode code)
{
    switch(code)
    {
        case HttpStatusCode::Continue: return "Oh good heavens, Continue!!!";
        case HttpStatusCode::SwitchingProtocols: return "Oh good heavens, SwitchingProtocols!!!";
        case HttpStatusCode::Processing: return "Oh good heavens, Processing!!!";
        case HttpStatusCode::EarlyHints: return "Oh good heavens, EarlyHints!!!";

        // 2xx
        case HttpStatusCode::OK: return "Oh good heavens, OK!!!";
        case HttpStatusCode::Created: return "Oh good heavens, Created!!!";
        case HttpStatusCode::Accepted: return "Oh good heavens, Accepted!!!";
        case HttpStatusCode::NonAuthoritativeInformation: return "Oh good heavens, NonAuthoritativeInformation!!!";
        case HttpStatusCode::NoContent: return "Oh good heavens, NoContent!!!";
        case HttpStatusCode::ResetContent: return "Oh good heavens, ResetContent!!!";
        case HttpStatusCode::PartialContent: return "Oh good heavens, PartialContent!!!";
        case HttpStatusCode::MultiStatus: return "Oh good heavens, MultiStatus!!!";
        case HttpStatusCode::AlreadyReported: return "Oh good heavens, AlreadyReported!!!";
        case HttpStatusCode::IMUsed: return "Oh good heavens, IMUsed!!!";

        // 3xx
        case HttpStatusCode::MultipleChoices: return "Oh good heavens, MultipleChoices!!!";
        case HttpStatusCode::MovedPermanently: return "Oh good heavens, MovedPermanently!!!";
        case HttpStatusCode::Found: return "Oh good heavens, Found!!!";
        case HttpStatusCode::SeeOther: return "Oh good heavens, SeeOther!!!";
        case HttpStatusCode::NotModified: return "Oh good heavens, NotModified!!!";
        case HttpStatusCode::UseProxy: return "Oh good heavens, UseProxy!!!";
        case HttpStatusCode::TemporaryRedirect: return "Oh good heavens, TemporaryRedirect!!!";
        case HttpStatusCode::PermanentRedirect: return "Oh good heavens, PermanentRedirect!!!";

        // 4xx
        case HttpStatusCode::BadRequest: return "Oh good heavens, BadRequest!!!";
        case HttpStatusCode::Unauthorized: return "Oh good heavens, Unauthorized!!!";
        case HttpStatusCode::PaymentRequired: return "Oh good heavens, PaymentRequired!!!";
        case HttpStatusCode::Forbidden: return "Oh good heavens, Forbidden!!!";
        case HttpStatusCode::NotFound: return "Oh good heavens, NotFound!!!";
        case HttpStatusCode::MethodNotAllowed: return "Oh good heavens, MethodNotAllowed!!!";
        case HttpStatusCode::NotAcceptable: return "Oh good heavens, NotAcceptable!!!";
        case HttpStatusCode::ProxyAuthenticationRequired: return "Oh good heavens, ProxyAuthenticationRequired!!!";
        case HttpStatusCode::RequestTimeout: return "Oh good heavens, RequestTimeout!!!";
        case HttpStatusCode::Conflict: return "Oh good heavens, Conflict!!!";
        case HttpStatusCode::Gone: return "Oh good heavens, Gone!!!";
        case HttpStatusCode::LengthRequired: return "Oh good heavens, LengthRequired!!!";
        case HttpStatusCode::PreconditionFailed: return "Oh good heavens, PreconditionFailed!!!";
        case HttpStatusCode::PayloadTooLarge: return "Oh good heavens, PayloadTooLarge!!!";
        case HttpStatusCode::URITooLong: return "Oh good heavens, URITooLong!!!";
        case HttpStatusCode::UnsupportedMediaType: return "Oh good heavens, UnsupportedMediaType!!!";
        case HttpStatusCode::RangeNotSatisfiable: return "Oh good heavens, RangeNotSatisfiable!!!";
        case HttpStatusCode::ExpectationFailed: return "Oh good heavens, ExpectationFailed!!!";
        case HttpStatusCode::ImATeapot: return "Oh good heavens, ImATeapot!!!";
        case HttpStatusCode::MisdirectedRequest: return "Oh good heavens, MisdirectedRequest!!!";
        case HttpStatusCode::UnprocessableEntity: return "Oh good heavens, UnprocessableEntity!!!";
        case HttpStatusCode::Locked: return "Oh good heavens, Locked!!!";
        case HttpStatusCode::FailedDependency: return "Oh good heavens, FailedDependency!!!";
        case HttpStatusCode::TooEarly: return "Oh good heavens, TooEarly!!!";
        case HttpStatusCode::UpgradeRequired: return "Oh good heavens, UpgradeRequired!!!";
        case HttpStatusCode::PreconditionRequired: return "Oh good heavens, PreconditionRequired!!!";
        case HttpStatusCode::TooManyRequests: return "Oh good heavens, TooManyRequests!!!";
        case HttpStatusCode::RequestHeaderFieldsTooLarge: return "Oh good heavens, RequestHeaderFieldsTooLarge!!!";
        case HttpStatusCode::UnavailableForLegalReasons: return "Oh good heavens, UnavailableForLegalReasons!!!";

        // 5xx
        case HttpStatusCode::InternalServerError: return "Oh good heavens, InternalServerError!!!";
        case HttpStatusCode::NotImplemented: return "Oh good heavens, NotImplemented!!!";
        case HttpStatusCode::BadGateway: return "Oh good heavens, BadGateway!!!";
        case HttpStatusCode::ServiceUnavailable: return "Oh good heavens, ServiceUnavailable!!!";
        case HttpStatusCode::GatewayTimeout: return "Oh good heavens, GatewayTimeout!!!";
        case HttpStatusCode::HTTPVersionNotSupported: return "Oh good heavens, HTTPVersionNotSupported!!!";
        case HttpStatusCode::VariantAlsoNegotiates: return "Oh good heavens, VariantAlsoNegotiates!!!";
        case HttpStatusCode::InsufficientStorage: return "Oh good heavens, InsufficientStorage!!!";
        case HttpStatusCode::LoopDetected: return "Oh good heavens, LoopDetected!!!";
        case HttpStatusCode::NotExtended: return "Oh good heavens, NotExtended!!!";
        case HttpStatusCode::NetworkAuthenticationRequired: return "Oh good heavens, NetworkAuthenticationRequired!!!";
        default: return "Oh good heavens, How did you get here?!!!!";
    }
}

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
	TransferEncoding,
};

std::string headerKeyToString(HeaderKey key);
std::ostream &operator<<(std::ostream &os, HeaderKey key);