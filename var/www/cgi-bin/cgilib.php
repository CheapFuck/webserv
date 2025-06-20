<?php
    enum HttpStatusCode: int {
        // 1xx
        case Continue = 100;
        case SwitchingProtocols = 101;
        case Processing = 102;
        case EarlyHints = 103;

        // 2xx
        case OK = 200;
        case Created = 201;
        case Accepted = 202;
        case NonAuthoritativeInformation = 203;
        case NoContent = 204;
        case ResetContent = 205;
        case PartialContent = 206;
        case MultiStatus = 207;
        case AlreadyReported = 208;
        case IMUsed = 226;

        // 3xx
        case MultipleChoices = 300;
        case MovedPermanently = 301;
        case Found = 302;
        case SeeOther = 303;
        case NotModified = 304;
        case UseProxy = 305;
        case TemporaryRedirect = 307;
        case PermanentRedirect = 308;

        // 4xx
        case BadRequest = 400;
        case Unauthorized = 401;
        case PaymentRequired = 402;
        case Forbidden = 403;
        case NotFound = 404;
        case MethodNotAllowed = 405;
        case NotAcceptable = 406;
        case ProxyAuthenticationRequired = 407;
        case RequestTimeout = 408;
        case Conflict = 409;
        case Gone = 410;
        case LengthRequired = 411;
        case PreconditionFailed = 412;
        case PayloadTooLarge = 413;
        case URITooLong = 414;
        case UnsupportedMediaType = 415;
        case RangeNotSatisfiable = 416;
        case ExpectationFailed = 417;
        case ImATeapot = 418;
        case MisdirectedRequest = 421;
        case UnprocessableEntity = 422;
        case Locked = 423;
        case FailedDependency = 424;
        case TooEarly = 425;
        case UpgradeRequired = 426;
        case PreconditionRequired = 428;
        case TooManyRequests = 429;
        case RequestHeaderFieldsTooLarge = 431;
        case UnavailableForLegalReasons = 451;

        // 5xx
        case InternalServerError = 500;
        case NotImplemented = 501;
        case BadGateway = 502;
        case ServiceUnavailable = 503;
        case GatewayTimeout = 504;
        case HTTPVersionNotSupported = 505;
        case VariantAlsoNegotiates = 506;
        case InsufficientStorage = 507;
        case LoopDetected = 508;
        case NotExtended = 510;
        case NetworkAuthenticationRequired = 511;
    }

    function getStatusCodeReason(HttpStatusCode $code): string {
        return match($code) {
            // 1xx
            HttpStatusCode::Continue => "Continue",
            HttpStatusCode::SwitchingProtocols => "Switching Protocols",
            HttpStatusCode::Processing => "Processing",
            HttpStatusCode::EarlyHints => "Early Hints",

            // 2xx
            HttpStatusCode::OK => "OK",
            HttpStatusCode::Created => "Created",
            HttpStatusCode::Accepted => "Accepted",
            HttpStatusCode::NonAuthoritativeInformation => "Non-Authoritative Information",
            HttpStatusCode::NoContent => "No Content",
            HttpStatusCode::ResetContent => "Reset Content",
            HttpStatusCode::PartialContent => "Partial Content",
            HttpStatusCode::MultiStatus => "Multi-Status",
            HttpStatusCode::AlreadyReported => "Already Reported",
            HttpStatusCode::IMUsed => "IM Used",

            // 3xx
            HttpStatusCode::MultipleChoices => "Multiple Choices",
            HttpStatusCode::MovedPermanently => "Moved Permanently",
            HttpStatusCode::Found => "Found",
            HttpStatusCode::SeeOther => "See Other",
            HttpStatusCode::NotModified => "Not Modified",
            HttpStatusCode::UseProxy => "Use Proxy",
            HttpStatusCode::TemporaryRedirect => "Temporary Redirect",
            HttpStatusCode::PermanentRedirect => "Permanent Redirect",

            // 4xx
            HttpStatusCode::BadRequest => "Bad Request",
            HttpStatusCode::Unauthorized => "Unauthorized",
            HttpStatusCode::PaymentRequired => "Payment Required",
            HttpStatusCode::Forbidden => "Forbidden",
            HttpStatusCode::NotFound => "Not Found",
            HttpStatusCode::MethodNotAllowed => "Method Not Allowed",
            HttpStatusCode::NotAcceptable => "Not Acceptable",
            HttpStatusCode::ProxyAuthenticationRequired => "Proxy Authentication Required",
            HttpStatusCode::RequestTimeout => "Request Timeout",
            HttpStatusCode::Conflict => "Conflict",
            HttpStatusCode::Gone => "Gone",
            HttpStatusCode::LengthRequired => "Length Required",
            HttpStatusCode::PreconditionFailed => "Precondition Failed",
            HttpStatusCode::PayloadTooLarge => "Payload Too Large",
            HttpStatusCode::URITooLong => "URI Too Long",
            HttpStatusCode::UnsupportedMediaType => "Unsupported Media Type",
            HttpStatusCode::RangeNotSatisfiable => "Range Not Satisfiable",
            HttpStatusCode::ExpectationFailed => "Expectation Failed",
            HttpStatusCode::ImATeapot => "I'm a teapot",
            HttpStatusCode::MisdirectedRequest => "Misdirected Request",
            HttpStatusCode::UnprocessableEntity => "Unprocessable Entity",
            HttpStatusCode::Locked => "Locked",
            HttpStatusCode::FailedDependency => "Failed Dependency",
            HttpStatusCode::TooEarly => "Too Early",
            HttpStatusCode::UpgradeRequired => "Upgrade Required",
            HttpStatusCode::PreconditionRequired => "Precondition Required",
            HttpStatusCode::TooManyRequests => "Too Many Requests",
            HttpStatusCode::RequestHeaderFieldsTooLarge => "Request Header Fields Too Large",
            HttpStatusCode::UnavailableForLegalReasons => "Unavailable For Legal Reasons",

            // 5xx
            HttpStatusCode::InternalServerError => "Internal Server Error",
            HttpStatusCode::NotImplemented => "Not Implemented",
            HttpStatusCode::BadGateway => "Bad Gateway",
            HttpStatusCode::ServiceUnavailable => "Service Unavailable",
            HttpStatusCode::GatewayTimeout => "Gateway Timeout",
            HttpStatusCode::HTTPVersionNotSupported => "HTTP Version Not Supported",
            HttpStatusCode::VariantAlsoNegotiates => "Variant Also Negotiates",
            HttpStatusCode::InsufficientStorage => "Insufficient Storage",
            HttpStatusCode::LoopDetected => "Loop Detected",
            HttpStatusCode::NotExtended => "Not Extended",
            HttpStatusCode::NetworkAuthenticationRequired => "Network Authentication Required",
            
            default => "Unknown Status Code"
        };
    }

?>