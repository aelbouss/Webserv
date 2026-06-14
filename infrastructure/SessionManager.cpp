#include "../includes/SessionManager.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>

static const size_t kSessionIdBytes = 16;
static const size_t kSessionIdHexLen = kSessionIdBytes * 2;

SessionManager::SessionManager()
	: _cookieName("WEBSERVSID"), _ttlSeconds(3600), _maxSessions(10000)
{
}

const std::string& SessionManager::getCookieName() const
{
	return _cookieName;
}

void SessionManager::setTtlSeconds(size_t seconds)
{
	_ttlSeconds = seconds;
}


//cookie : theme=dark WEBSEREVID=1225ffd11a12a1155 ...etc
SessionManager::Result SessionManager::getOrCreate(const std::string& cookieHeader)
{
	Result result;
	result.isNew = false;

	time_t now = std::time(NULL);
	purgeExpired(now);

	std::string candidate = extractCookieValue(cookieHeader, _cookieName);
	if (!candidate.empty()) // && isHexString(candidate)
	{
		std::map<std::string, Session>::iterator it = _sessions.find(candidate);
		if (it != _sessions.end())
		{
			it->second.lastAccess = now;
			result.id = candidate;
			return result;
		}
	}

	std::string sid;
	for (int attempt = 0; attempt < 5; ++attempt)
	{
		sid = generateSessionId();
		if (_sessions.find(sid) == _sessions.end())
			break;
	}

	Session session;
	session.createdAt = now;
	session.lastAccess = now;
	_sessions[sid] = session;

	result.id = sid;
	result.isNew = true;
	return result;
}

std::string SessionManager::buildSetCookieHeader(const std::string& sessionId) const
{
	std::ostringstream out;
	out << _cookieName << "=" << sessionId
		<< "; Path=/; HttpOnly; SameSite=Lax; Max-Age=" << _ttlSeconds;
	return out.str();
}

void SessionManager::purgeExpired(time_t now)
{
	if (_ttlSeconds == 0)
		return;

	std::map<std::string, Session>::iterator it = _sessions.begin();
	while (it != _sessions.end())
	{
		time_t age = now - it->second.lastAccess;
		if (age > static_cast<time_t>(_ttlSeconds))
		{
			std::map<std::string, Session>::iterator victim = it;
			++it;
			_sessions.erase(victim);
		}
		else
		{
			++it;
		}
	}

	if (_sessions.size() <= _maxSessions)
		return;
}

std::string SessionManager::trimCopy(const std::string& s)
{
	size_t begin = 0;
	size_t end = s.size();
	while (begin < end && (s[begin] == ' ' || s[begin] == '\t' || s[begin] == '\r' || s[begin] == '\n'))
		++begin;
	while (end > begin && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n'))
		--end;
	return s.substr(begin, end - begin);
}

// bool SessionManager::isHexString(const std::string& value)
// {
// 	if (value.size() != kSessionIdHexLen)
// 		return false;
// 	for (size_t i = 0; i < value.size(); ++i)
// 	{
// 		char c = value[i];
// 		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
// 			return false;
// 	}
// 	return true;
// }

std::string SessionManager::extractCookieValue(const std::string& cookieHeader, const std::string& name) const
{
	size_t pos = 0;
	while (pos < cookieHeader.size())
	{
		size_t end = cookieHeader.find(';', pos);
		std::string token = (end == std::string::npos)
			? cookieHeader.substr(pos)
			: cookieHeader.substr(pos, end - pos);
		token = trimCopy(token);
		if (!token.empty())
		{
			size_t eq = token.find('=');
			if (eq != std::string::npos)
			{
				std::string key = trimCopy(token.substr(0, eq));
				std::string value = token.substr(eq + 1);
				if (key == name)
					return value;
			}
		}
		if (end == std::string::npos)
			break;
		pos = end + 1;
	}
	return "";
}

//////////////////////////////////////////////////////////////////////////////////////


// create map that store key and value from cookie header 
//


/*

cookie : WEBSEREVID=122fgd22fg5fdg1f; theme=dark;.....


*/


std::map<std::string, std::string> SessionManager::parseCookie(const std::string& cookieHeader)
{
	std::map<std::string, std::string> res;
	size_t pos = 0;
	while (pos < cookieHeader.size())
	{
		size_t end = cookieHeader.find(';', pos);
		std::string token = (end == std::string::npos)
			? cookieHeader.substr(pos)
			: cookieHeader.substr(pos, end - pos);
		token = trimCopy(token);
		if (!token.empty())
		{
			size_t eq = token.find('=');
			if (eq != std::string::npos)
			{
				std::string key = trimCopy(token.substr(0, eq));
				std::string value = token.substr(eq + 1);
				res[key] = value;
			}
		}
		if (end == std::string::npos)
			break;
		pos = end + 1;
	}
	return res;
}

//////////////////////////////////////////////////////////////////////////////////////


std::string SessionManager::generateSessionId() const
{
	unsigned char bytes[kSessionIdBytes];
	bool ok = false;
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd >= 0)
	{
		ssize_t n = read(fd, bytes, sizeof(bytes));
		close(fd);
		if (n == static_cast<ssize_t>(sizeof(bytes)))
			ok = true;
	}

	if (!ok)
	{
		static bool seeded = false;
		if (!seeded)
		{
			seeded = true;
			std::srand(static_cast<unsigned int>(std::time(NULL)) ^ static_cast<unsigned int>(getpid()));
		}
		for (size_t i = 0; i < sizeof(bytes); ++i)
			bytes[i] = static_cast<unsigned char>(std::rand() % 256);
	}

	std::ostringstream out;
	out << std::hex << std::setfill('0');
	for (size_t i = 0; i < sizeof(bytes); ++i)
		out << std::setw(2) << static_cast<int>(bytes[i]);
	return out.str();
}
