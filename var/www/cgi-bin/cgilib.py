import http.cookies
import logging
import typing
import fcntl
import json
import enum
import sys
import os

CGI_COOKIE_PREFIX = 'HTTP_COOKIE_'
CGI_SESSION_FILE_KEY = 'HTTP_SESSION_FILE'

logging.basicConfig(filename='python.log', level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

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
				with open(filename, 'w') as f:
					json.dump({}, f)
			with open(filename, 'r') as file:
				fcntl.flock(file, fcntl.LOCK_EX)
				try:
					self._data = json.load(file)
					return True
				except json.JSONDecodeError:
					self._data = {}
					fcntl.flock(file, fcntl.LOCK_UN)
					return False
		
		return self._is_loaded and bool(self._filename)

	def save(self) -> None:
		"""Saves the session data to the specified file."""
		if not self._is_loaded or not self._filename:
			raise RuntimeError("Session data not loaded or filename not set. Call load() with a valid filename first.")
		
		with open(self._filename, 'w') as file:
			json.dump(self._data, file)
			fcntl.flock(file, fcntl.LOCK_UN)
			self._is_loaded = False

	def __getitem__(self, key: str) -> typing.Any:
		if not self._is_loaded:
			raise KeyError(f"Session data not loaded. Call load() with a valid filename first.")
		return self._data[key]

	def __setitem__(self, key: str, value: typing.Any) -> None:
		if not self._is_loaded:
			raise KeyError(f"Session data not loaded. Call load() with a valid filename first.")
		self._data[key] = value

	def __delitem__(self, key: str) -> None:
		if not self._is_loaded:
			raise KeyError(f"Session data not loaded. Call load() with a valid filename first.")
		del self._data[key]

	def __contains__(self, key: str) -> bool:
		if not self._is_loaded:
			raise KeyError(f"Session data not loaded. Call load() with a valid filename first.")
		return key in self._data
	
	def __iter__(self) -> typing.Iterator[typing.Tuple[str, typing.Any]]:
		if not self._is_loaded:
			raise KeyError(f"Session data not loaded. Call load() with a valid filename first.")
		return iter(self._data.items())

class CGIClient:
	def __init__(self) -> None:
		self.session: SessionHandler = SessionHandler(os.environ.get(CGI_SESSION_FILE_KEY))
		self._cgiStatus: CGIStatus = CGIStatus.INIT
		self._body: str | None = None

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
			raise RuntimeError("Cannot set headers after sending body.")
		self._cgiStatus = CGIStatus.SEND_HEADERS

		print(f"{name}: {value}", flush=True, end='\r\n')

	def setCookie(self, cookie: http.cookies.SimpleCookie) -> None:
		"""Sets a cookie to be sent in the response."""
		if self._cgiStatus > CGIStatus.SEND_HEADERS:
			raise RuntimeError("Cannot set cookies after sending body.")
		self._cgiStatus = CGIStatus.SEND_HEADERS

		print(cookie.output(header='', sep=''), flush=True, end='\r\n')

	def sendBody(self, body: str) -> None:
		"""Sends the body of the response."""
		if self._cgiStatus < CGIStatus.SEND_BODY:
			# self.setHeader('Content-Length', str(len(body)))
			print('', end='\r\n', flush=True)
		if self._cgiStatus == CGIStatus.SEND_BODY:
			raise RuntimeError("Body cannot be sent multiple times.")
		elif self._cgiStatus > CGIStatus.SEND_BODY:
			raise RuntimeError("Cannot send body after ending the response.")
		self._cgiStatus = CGIStatus.SEND_BODY

		print(body, flush=True)
