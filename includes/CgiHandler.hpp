#pragma once

#include "WebServer.hpp"
#include "request.hpp"

typedef Request HttpRequest;

class CgiHandler {
	private:
		std::map<std::string, std::string>	_env;
		char**								_ch_env;
		char**								_argv;
		int									_exit_status;
		std::string							_cgi_path;
		pid_t								_cgi_pid;

	public:
		int	pipe_in[2];
		int	pipe_out[2];

		CgiHandler();
		CgiHandler(std::string path);
		~CgiHandler();

		void initEnvBasic(Request& req, const std::string& scriptPath,
						  const std::string& requestUri,
						  const std::string& queryString);
		void initEnvFromLocation(Request& req, const Location& location);

		// Non-blocking CGI: fork the child and make the parent pipe ends
		// non-blocking. Returns 0 on success, or an HTTP error code (500).
		// All actual reading/writing is driven by the main poll() loop.
		short start(Request& request);
		int   getReadFd() const;    // parent read end  (pipe_out[0])
		int   getWriteFd() const;   // parent write end (pipe_in[1])
		void  closeWriteFd();       // close write end (signals EOF to child)
		void  killChild();          // SIGKILL + reap (timeout / abort)
		int   reapChild();          // reap child, return exit status

		// Disable copying to avoid shallow-copying raw pointers and double-free.
		private:
			CgiHandler(CgiHandler const &other);
			CgiHandler &operator=(CgiHandler const &rhs);

		public:
		void clear();
		std::string setCookie(const std::string& str);

		void setCgiPid(pid_t cgi_pid);
		void setCgiPath(const std::string &cgi_path);

		const std::map<std::string, std::string> &getEnv() const;
		const pid_t &getCgiPid() const;
		const std::string &getCgiPath() const;

		std::string	getAfter(const std::string& path, char delim);
		std::string	getBefore(const std::string& path, char delim);
		std::string	getPathInfo(std::string& path, std::vector<std::string> extensions);
		int	countCookies(const std::string& str);
		int findStart(const std::string path, const std::string delim);
		std::string decode(std::string &path);
};