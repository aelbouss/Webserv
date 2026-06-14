#pragma once

#include <map>
#include <string>
#include <ctime>
#include <cstddef>

class SessionManager
{
	public:
		struct Result
		{
			std::string id;
			bool isNew;
		};

		SessionManager();

		Result getOrCreate(const std::string& cookieHeader);
		std::string buildSetCookieHeader(const std::string& sessionId) const;
		std::map<std::string, std::string> parseCookie(const std::string& cookieHeader);
		const std::string& getCookieName() const;
		void setTtlSeconds(size_t seconds);

	private:
		struct Session
		{
			time_t createdAt;
			time_t lastAccess;
		};

		std::map<std::string, Session> _sessions;
		std::string _cookieName;
		size_t _ttlSeconds;
		size_t _maxSessions;
		
		static std::string trimCopy(const std::string& s);
		static bool isHexString(const std::string& value);
		std::string extractCookieValue(const std::string& cookieHeader, const std::string& name) const;
		std::string generateSessionId() const;
		void purgeExpired(time_t now);
};
