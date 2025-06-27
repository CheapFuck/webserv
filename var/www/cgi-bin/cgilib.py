import http.cookies
import weakref
import typing
import json
import enum
import sys
import os

from enum import Enum

CGI_COOKIE_PREFIX = 'HTTP_COOKIE_'
CGI_SESSION_FILE_KEY = 'HTTP_SESSION_FILE'

class HttpStatusCode(Enum):
    # 1xx
    Continue = 100
    SwitchingProtocols = 101
    Processing = 102
    EarlyHints = 103

    # 2xx
    OK = 200
    Created = 201
    Accepted = 202
    NonAuthoritativeInformation = 203
    NoContent = 204
    ResetContent = 205
    PartialContent = 206
    MultiStatus = 207
    AlreadyReported = 208
    IMUsed = 226

    # 3xx
    MultipleChoices = 300
    MovedPermanently = 301
    Found = 302
    SeeOther = 303
    NotModified = 304
    UseProxy = 305
    TemporaryRedirect = 307
    PermanentRedirect = 308

    # 4xx
    BadRequest = 400
    Unauthorized = 401
    PaymentRequired = 402
    Forbidden = 403
    NotFound = 404
    MethodNotAllowed = 405
    NotAcceptable = 406
    ProxyAuthenticationRequired = 407
    RequestTimeout = 408
    Conflict = 409
    Gone = 410
    LengthRequired = 411
    PreconditionFailed = 412
    PayloadTooLarge = 413
    URITooLong = 414
    UnsupportedMediaType = 415
    RangeNotSatisfiable = 416
    ExpectationFailed = 417
    ImATeapot = 418
    MisdirectedRequest = 421
    UnprocessableEntity = 422
    Locked = 423
    FailedDependency = 424
    TooEarly = 425
    UpgradeRequired = 426
    PreconditionRequired = 428
    TooManyRequests = 429
    RequestHeaderFieldsTooLarge = 431
    UnavailableForLegalReasons = 451

    # 5xx
    InternalServerError = 500
    NotImplemented = 501
    BadGateway = 502
    ServiceUnavailable = 503
    GatewayTimeout = 504
    HTTPVersionNotSupported = 505
    VariantAlsoNegotiates = 506
    InsufficientStorage = 507
    LoopDetected = 508
    NotExtended = 510
    NetworkAuthenticationRequired = 511

    def getAsStr(self) -> str:
        match self.value:
            # 1xx
            case 100: return 'Continue'
            case 101: return 'Switching Protocols'
            case 102: return 'Processing'
            case 103: return 'Early Hints'

            # 2xx
            case 200: return 'OK'
            case 201: return 'Created'
            case 202: return 'Accepted'
            case 203: return 'Non-Authoritative Information'
            case 204: return 'No Content'
            case 205: return 'Reset Content'
            case 206: return 'Partial Content'
            case 207: return 'Multi-Status'
            case 208: return 'Already Reported'
            case 226: return 'IM Used'

            # 3xx
            case 300: return 'Multiple Choices'
            case 301: return 'Moved Permanently'
            case 302: return 'Found'
            case 303: return 'See Other'
            case 304: return 'Not Modified'
            case 305: return 'Use Proxy'
            case 307: return 'Temporary Redirect'
            case 308: return 'Permanent Redirect'

            # 4xx
            case 400: return 'Bad Request'
            case 401: return 'Unauthorized'
            case 402: return 'Payment Required'
            case 403: return 'Forbidden'
            case 404: return 'Not Found'
            case 405: return 'Method Not Allowed'
            case 406: return 'Not Acceptable'
            case 407: return 'Proxy Authentication Required'
            case 408: return 'Request Timeout'
            case 409: return 'Conflict'
            case 410: return 'Gone'
            case 411: return 'Length Required'
            case 412: return 'Precondition Failed'
            case 413: return 'Payload Too Large'
            case 414: return 'URI Too Long'
            case 415: return 'Unsupported Media Type'
            case 416: return 'Range Not Satisfiable'
            case 417: return 'Expectation Failed'
            case 418: return 'I\'m a teapot'
            case 421: return 'Misdirected Request'
            case 422: return 'Unprocessable Entity'
            case 423: return 'Locked'
            case 424: return 'Failed Dependency'
            case 425: return 'Too Early'
            case 426: return 'Upgrade Required'
            case 428: return 'Precondition Required'
            case 429: return 'Too Many Requests'
            case 431: return 'Request Header Fields Too Large'
            case 451: return 'Unavailable For Legal Reasons'

            # 5xx
            case 500: return 'Internal Server Error'
            case 501: return 'Not Implemented'
            case 502: return 'Bad Gateway'
            case 503: return 'Service Unavailable'
            case 504: return 'Gateway Timeout'
            case 505: return 'HTTP Version Not Supported'
            case 506: return 'Variant Also Negotiates'
            case 507: return 'Insufficient Storage'
            case 508: return 'Loop Detected'
            case 510: return 'Not Extended'
            case 511: return 'Network Authentication Required'

            case _: 
                print(f'ERROR: Unknown HTTP Status Code: {code}')
                return 'Unknown Status Code'

class CGIStatus(enum.Enum):
    INIT = 0
    SEND_HEADERS = 1
    SEND_BODY = 2
    END = 3

    def __lt__(self, other: 'CGIStatus') -> bool:
        return self.value < other.value

    def __le__(self, other: 'CGIStatus') -> bool:
        return self.value <= other.value

    def __gt__(self, other: 'CGIStatus') -> bool:
        return self.value > other.value

    def __ge__(self, other: 'CGIStatus') -> bool:
        return self.value >= other.value

class SessionHandler:
    def __init__(self, filename: str | None = None) -> None:
        self._is_loaded = False
        self._filename: str | None = filename
        self._data: dict[str, typing.Any] = {}
    
    def __enter__(self) -> 'SessionHandler':
        """Context manager entry method."""
        if self._filename is None or not self.load(self._filename):
            raise RuntimeError(f"Failed to load session data from {self._filename}.")
        return self

    def __exit__(self, exc_type: typing.Optional[typing.Type[BaseException]], exc_value: typing.Optional[BaseException], traceback: typing.Optional[typing.Any]) -> None:
        """Context manager exit method."""
        self.save()
    
    def load(self, filename: str) -> bool:
        """Loads session data from the environment. Returns True if the session was loaded successfully, False otherwise."""
        if not self._is_loaded and filename:
            self._is_loaded = True

            # Create the file if it does not exist
            if not os.path.exists(filename):
                self._data = {}
                return True
            
            with open(filename, 'r') as file:
                self._data = json.load(file)

        return self._is_loaded and bool(self._filename)

    def save(self) -> None:
        """Saves the session data to the specified file."""
        if not self._is_loaded or not self._filename:
            raise RuntimeError('Session data not loaded or filename not set. Call load() with a valid filename first.')
        
        with open(self._filename, 'w') as file:
            json.dump(self._data, file)
            self._is_loaded = False

    def __getitem__(self, key: str) -> typing.Any:
        if not self._is_loaded:
            raise KeyError(f'Session data not loaded. Call load() with a valid filename first.')
        return self._data[key]

    def __setitem__(self, key: str, value: typing.Any) -> None:
        if not self._is_loaded:
            raise KeyError(f'Session data not loaded. Call load() with a valid filename first.')
        self._data[key] = value

    def __delitem__(self, key: str) -> None:
        if not self._is_loaded:
            raise KeyError(f'Session data not loaded. Call load() with a valid filename first.')
        del self._data[key]

    def __contains__(self, key: str) -> bool:
        if not self._is_loaded:
            raise KeyError(f'Session data not loaded. Call load() with a valid filename first.')
        return key in self._data
    
    def __iter__(self) -> typing.Iterator[typing.Tuple[str, typing.Any]]:
        if not self._is_loaded:
            raise KeyError(f'Session data not loaded. Call load() with a valid filename first.')
        return iter(self._data.items())
    
    def get(self, key: str, default: typing.Any = None) -> typing.Any:
        """Returns the value for the given key, or the default value if the key does not exist."""
        if not self._is_loaded:
            raise KeyError(f'Session data not loaded. Call load() with a valid filename first.')
        return self._data.get(key, default)

class CGIClient:
    def __init__(self) -> None:
        self.session: SessionHandler = SessionHandler(os.environ.get(CGI_SESSION_FILE_KEY))
        self._cgiStatus: CGIStatus = CGIStatus.INIT
        self._body: str | None = None

        self.pathParameters: list[str] = os.environ.get('PATH_INFO', '').strip('/').split('/') if 'PATH_INFO' in os.environ else []
        self.queryParameters: dict[str, str] = {k: v for k, v in (param.split('=') for param in os.environ.get('QUERY_STRING', '').split('&') if '=' in param)}

        self.routes: dict[str, typing.Callable[[], None]] = {}
    
    def route(self, path: str) -> typing.Callable[[typing.Callable[[], None]], typing.Callable[[], None]]:
        """Decorator to register a route with a specific path."""
        def decorator(func: typing.Callable[[], None]) -> typing.Callable[[], None]:
            self.routes[path] = func
            return func
        return decorator
    
    def run(self) -> None:
        """Runs the CGI client, executing the route based on the path parameters."""
        if not self.routes:
            raise RuntimeError('No routes provided.')

        best_path: str | None = None
        path_info = self.getEnvironmentVariable('PATH_INFO', '/') or '/'
        assert isinstance(path_info, str)

        for route in self.routes:
            if path_info.startswith(route) and (not best_path or len(route) > len(best_path)):
                best_path = route

        if best_path is None:
            self.setStatus(HttpStatusCode.NotFound)
            return

        count: int = len([s for s in best_path.split('/') if len(s)])
        for _ in range(count):
            if self.pathParameters:
                self.pathParameters.pop(0)

        self.routes[best_path]()

    def getPathParameter(self, index: int, default: str | None = None) -> str | None:
        """Returns the path parameter at the specified index. If the index is out of bounds, returns the default value."""
        if 0 <= index < len(self.pathParameters):
            return self.pathParameters[index]
        return default

    def getQueryParameter(self, name: str, default: str | None = None) -> str | None:
        """Returns the value of a query parameter by name. If no default value is provided and the parameter does not exist, returns None."""
        return self.queryParameters.get(name, default)

    def getCookie(self, name: str, default: str | None = None) -> str | None:
        """Returns the value of a cookie by name. If no default value is provided and the cookie does not exist, returns None."""
        return os.environ.get(CGI_COOKIE_PREFIX + name, default)

    def getEnvironmentVariable(self, name: str, default: str | None = None) -> str | None:
        """Returns the value of an environment variable by name. If no default value is provided and the variable does not exist, returns None."""
        return os.environ.get(name, default)

    def getBody(self) -> str | None:
        """Returns the body of the request. If the body is not already set, it attempts to read from stdin."""
        if self._body is None:
            try:
                content_length = int(os.environ.get('CONTENT_LENGTH', 0))
                if content_length > 0:
                    self._body = sys.stdin.read(content_length)
            except (ValueError, OSError):
                self._body = ''

        return self._body

    def setHeader(self, name: str, value: str) -> None:
        """Sets a header to be sent in the response."""
        if self._cgiStatus > CGIStatus.SEND_HEADERS:
            raise RuntimeError('Cannot set headers after sending body.')
        self._cgiStatus = CGIStatus.SEND_HEADERS

        print(f"{name}: {value}", flush=True, end='\r\n')
    
    def setStatus(self, status: HttpStatusCode) -> None:
        """Sets the HTTP status code for the response."""
        self.setHeader('Status', f'{status.value} {status.getAsStr()}')

    def setCookie(self, cookie: http.cookies.SimpleCookie) -> None:
        """Sets a cookie to be sent in the response."""
        if self._cgiStatus > CGIStatus.SEND_HEADERS:
            raise RuntimeError('Cannot set cookies after sending body.')
        self._cgiStatus = CGIStatus.SEND_HEADERS

        print(cookie.output(header='', sep=''), flush=True, end='\r\n')

    def sendBody(self, body: str) -> None:
        """Sends the body of the response."""
        if self._cgiStatus < CGIStatus.SEND_BODY:
            print('', end='\r\n', flush=True)
        elif self._cgiStatus > CGIStatus.SEND_BODY:
            raise RuntimeError('Cannot send body after ending the response.')
        self._cgiStatus = CGIStatus.SEND_BODY

        if body:
            print(body, flush=True, end='')

    def wrapUpResponse(self) -> None:
        if self._cgiStatus < CGIStatus.SEND_BODY:
            self.sendBody('')
        if self._cgiStatus < CGIStatus.END:
            print('', flush=True, end='\r\n')
        self._cgiStatus = CGIStatus.END

    def onDelete(self, callback: typing.Callable[[], None]) -> None:
        """Registers a callback to be called when the CGIClient is deleted."""
        weakref.finalize(self, callback)