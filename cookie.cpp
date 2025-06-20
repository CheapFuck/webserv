#include "sessionManager.hpp"
#include "cookie.hpp"
#include "print.hpp"
#include "Utils.hpp"

#include <sstream>
#include <vector>

/// @brief Creates a Cookie object with the specified name and value.
/// @param name Name of the cookie.
/// @param value Value of the cookie.
/// @return A Cookie object initialized with the given name and value.
Cookie Cookie::create(const std::string &name, const std::string &value) {
	Cookie cookie;
	cookie.
		setName(name).
		setValue(value);
	return (cookie);
}

/// @brief Generates a string representation of the cookie for HTTP header initialization.
/// This string can be used in HTTP responses to set the cookie in the client's browser.
std::string Cookie::getHeaderInitializationString() const {
	std::ostringstream ss;
	ss << getName() << "=" << getValue();
	if (getMaxAge() > 0)
		ss << "; Max-Age=" << getMaxAge();
	if (!getPath().str().empty())
		ss << "; Path=" << getPath().str();
	DEBUG("Cookie header initialization string: " << ss.str());
	return (ss.str());
}

/// @brief Parses a cookie header value and creates a vector of Cookie objects.
/// @param header_value The value of the Set-Cookie header from an HTTP response.
/// @return A vector of Cookie objects parsed from the header value.
std::vector<Cookie> Cookie::createAllFromHeader(const std::string &header_value) {
	std::vector<Cookie> cookies;

	std::vector<std::string> split = Utils::split(header_value, ';');
	for (const std::string &cookie_str : split) {
		size_t eq_pos = cookie_str.find('=');
		if (eq_pos == std::string::npos) {
			DEBUG("Invalid cookie format: " << cookie_str);
			continue;
		}
		std::string name = Utils::trim(cookie_str.substr(0, eq_pos));
		std::string value = Utils::trim(cookie_str.substr(eq_pos + 1));
		cookies.push_back(Cookie::create(name, value));
	}

	return (cookies);
}

/// @brief Creates a session cookie with the specified session ID.
/// @param sessionId The session ID to be stored in the cookie.
Cookie Cookie::createSessionCookie(const std::string &sessionId) {
	Cookie cookie;
	cookie.setName(SESSION_COOKIE_NAME)
		  .setValue(sessionId)
		  .setMaxAge(SESSION_MAX_STORAGE_AGE);
	return (cookie);
}

/// @brief Retrieves a cookie by name from a vector of cookies.
/// @param cookies The vector of Cookie objects to search.
/// @param name The name of the cookie to find.
const Cookie *Cookie::getCookie(const std::vector<Cookie> &cookies, const std::string &name) {
	for (const Cookie &cookie : cookies) {
		if (cookie.getName() == name)
			return (&cookie);
	}
	return nullptr;
}

/// @brief Retrieves a cookie by name from a Request object.
/// @param request The Request object containing cookies.
/// @param name The name of the cookie to find.
const Cookie *Cookie::getCookie(const Request &request, const std::string &name) {
	return getCookie(request.getCookies(), name);
}
