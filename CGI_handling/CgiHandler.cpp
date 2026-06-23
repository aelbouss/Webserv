#include "../includes/CgiHandler.hpp"
#include "../includes/request.hpp"
#include <sys/wait.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <ctime>

static std::string normalizeHeaderKey(const std::string& name)
{
	std::string out;
	out.reserve(name.size());
	for (size_t i = 0; i < name.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(name[i]);
		if (c == '-')
			out += '_';
		else
			out += static_cast<char>(std::toupper(c));
	}
	return out;
}


CgiHandler::CgiHandler() {
	this->_cgi_pid = -1;
	this->_exit_status = 0;
	this->_cgi_path = "";
	this->_ch_env = NULL;
	this->_argv = NULL;
	this->pipe_in[0] = -1; this->pipe_in[1] = -1;
	this->pipe_out[0] = -1; this->pipe_out[1] = -1;
}

CgiHandler::CgiHandler(std::string path)
{
	this->_cgi_pid = -1;
	this->_exit_status = 0;
	this->_cgi_path = path;
	this->_ch_env = NULL;
	this->_argv = NULL;
	this->pipe_in[0] = -1; this->pipe_in[1] = -1;
	this->pipe_out[0] = -1; this->pipe_out[1] = -1;
}

CgiHandler::~CgiHandler() {

	// Safety net: close any still-open parent pipe ends and reap the child
	// (the multiplexer normally does this, but guard against leaks/zombies).
	if (this->pipe_in[1] >= 0)
		close(this->pipe_in[1]);
	if (this->pipe_out[0] >= 0)
		close(this->pipe_out[0]);
	if (this->_cgi_pid > 0)
	{
		int status = 0;
		if (waitpid(this->_cgi_pid, &status, WNOHANG) == 0)
		{
			kill(this->_cgi_pid, SIGKILL);
			waitpid(this->_cgi_pid, &status, 0);
		}
	}
	if (this->_ch_env)
	{
		for (int i = 0; this->_ch_env[i]; i++)
			free(this->_ch_env[i]);
		free(this->_ch_env);
	}
	if (this->_argv)
	{
		for (int i = 0; this->_argv[i]; i++)
			free(_argv[i]);
		free(_argv);
	}
	this->_env.clear();
}

CgiHandler::CgiHandler(const CgiHandler &other)
{
    this->_env = other._env;
    this->_cgi_path = other._cgi_path;
    this->_cgi_pid = other._cgi_pid;
    this->_exit_status = other._exit_status;

    // Deep-copy environment array
    this->_ch_env = NULL;
    if (other._ch_env)
    {
        int cnt = 0;
        while (other._ch_env[cnt]) cnt++;
        this->_ch_env = (char **)calloc(cnt + 1, sizeof(char *));
        for (int i = 0; i < cnt; ++i)
            this->_ch_env[i] = strdup(other._ch_env[i]);
        this->_ch_env[cnt] = NULL;
    }

    // Deep-copy argv
    this->_argv = NULL;
    if (other._argv)
    {
        int cnt = 0;
        while (other._argv[cnt]) cnt++;
        this->_argv = (char **)malloc(sizeof(char *) * (cnt + 1));
        for (int i = 0; i < cnt; ++i)
            this->_argv[i] = strdup(other._argv[i]);
        this->_argv[cnt] = NULL;
    }
}

CgiHandler &CgiHandler::operator=(const CgiHandler &rhs)
{
    if (this != &rhs)
	{
		if (this->_ch_env)
		{
			for (int i = 0; this->_ch_env[i]; i++)
				free(this->_ch_env[i]);
			free(this->_ch_env);
			this->_ch_env = NULL;
		}
		if (this->_argv)
		{
			for (int i = 0; this->_argv[i]; i++)
				free(this->_argv[i]);
			free(this->_argv);
			this->_argv = NULL;
		}

		this->_env = rhs._env;
		this->_cgi_path = rhs._cgi_path;
		this->_cgi_pid = rhs._cgi_pid;
		this->_exit_status = rhs._exit_status;

		if (rhs._ch_env)
		{
			int cnt = 0;
			while (rhs._ch_env[cnt]) cnt++;
			this->_ch_env = (char **)calloc(cnt + 1, sizeof(char *));
			for (int i = 0; i < cnt; ++i)
				this->_ch_env[i] = strdup(rhs._ch_env[i]);
			this->_ch_env[cnt] = NULL;
		}

		if (rhs._argv)
		{
			int cnt = 0;
			while (rhs._argv[cnt]) cnt++;
			this->_argv = (char **)malloc(sizeof(char *) * (cnt + 1));
			for (int i = 0; i < cnt; ++i)
				this->_argv[i] = strdup(rhs._argv[i]);
			this->_argv[cnt] = NULL;
		}
	}
	return (*this);
}

/*Set functions */
void CgiHandler::setCgiPid(pid_t cgi_pid)
{
    this->_cgi_pid = cgi_pid;
}

void CgiHandler::setCgiPath(const std::string &cgi_path)
{
	this->_cgi_path = cgi_path;
}

/* Get functions */
const std::map<std::string, std::string> &CgiHandler::getEnv() const
{
    return (this->_env);
}

const pid_t &CgiHandler::getCgiPid() const
{
    return (this->_cgi_pid);
}

const std::string &CgiHandler::getCgiPath() const
{
    return (this->_cgi_path);
}

void CgiHandler::initEnvBasic(Request& req, const std::string& scriptPath,
						const std::string& requestUri,
						const std::string& queryString)
{
	this->_cgi_path = scriptPath;

	this->_env.clear();
	this->_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	this->_env["SCRIPT_FILENAME"] = scriptPath;
	this->_env["SCRIPT_NAME"] = requestUri;
	this->_env["REQUEST_METHOD"] = req.getMethodStr();
	this->_env["QUERY_STRING"] = queryString;
	this->_env["REQUEST_URI"] = requestUri + (queryString.empty() ? "" : "?" + queryString);
	this->_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	this->_env["REDIRECT_STATUS"] = "200";
	if (!req.getRemoteAddr().empty())
		this->_env["REMOTE_ADDR"] = req.getRemoteAddr();

	if (req.getMethod() == Request::POST)
	{
		std::stringstream out;
		out << req.getBody().size();
		this->_env["CONTENT_LENGTH"] = out.str();
		this->_env["CONTENT_TYPE"] = req.getHeader("content-type");
	}

	std::string host = req.getHeader("host");
	if (!host.empty())
	{
		size_t pos = host.find(':');
		this->_env["SERVER_NAME"] = (pos == std::string::npos) ? host : host.substr(0, pos);
		this->_env["SERVER_PORT"] = (pos == std::string::npos) ? "" : host.substr(pos + 1);
	}

	std::map<std::string, std::string> request_headers = req.getHeaders();
	for (std::map<std::string, std::string>::iterator it = request_headers.begin();
		 it != request_headers.end(); ++it)
	{
		std::string key = "HTTP_" + normalizeHeaderKey(it->first);
		_env[key] = it->second;
	}

	this->_ch_env = (char **)calloc(this->_env.size() + 1, sizeof(char *));
	std::map<std::string, std::string>::const_iterator it = this->_env.begin();
	for (int i = 0; it != this->_env.end(); it++, i++)
	{
		std::string tmp = it->first + "=" + it->second;
		this->_ch_env[i] = strdup(tmp.c_str());
	}

	this->_argv = (char **)malloc(sizeof(char *) * 2);
	this->_argv[0] = strdup(this->_cgi_path.c_str());
	this->_argv[1] = NULL;
}

void CgiHandler::initEnvFromLocation(Request& req, const Location& location)
{
	std::string extension;
	std::string ext_path;

	if (this->_cgi_path.empty() || this->_cgi_path.find(".") == std::string::npos)
		return;

	extension = this->_cgi_path.substr(this->_cgi_path.find("."));
	const std::map<std::string, std::string>& ext_map = location.getExtensionPath();
	std::map<std::string, std::string>::const_iterator it_path = ext_map.find(extension);
	if (it_path == ext_map.end())
		return;
	ext_path = it_path->second;

	this->_env.clear();
	this->_env["AUTH_TYPE"] = "Basic";
	this->_env["CONTENT_LENGTH"] = req.getHeader("content-length");
	this->_env["CONTENT_TYPE"] = req.getHeader("content-type");
	this->_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	this->_env["SCRIPT_NAME"] = this->_cgi_path;
	this->_env["SCRIPT_FILENAME"] = this->_cgi_path;
	std::string path = req.getPath();
	this->_env["PATH_INFO"] = getPathInfo(path, location.getCgiExtension());
	this->_env["PATH_TRANSLATED"] = location.getRootLocation() + (this->_env["PATH_INFO"].empty() ? "/" : this->_env["PATH_INFO"]);
	std::string query = req.getQuery();
	this->_env["QUERY_STRING"] = decode(query);
	if (!req.getRemoteAddr().empty())
		this->_env["REMOTE_ADDR"] = req.getRemoteAddr();
	int poz = findStart(req.getHeader("host"), ":");
	this->_env["SERVER_NAME"] = (poz > 0 ? req.getHeader("host").substr(0, poz) : "");
	this->_env["SERVER_PORT"] = (poz > 0 ? req.getHeader("host").substr(poz + 1, req.getHeader("host").size()) : "");
	this->_env["REQUEST_METHOD"] = req.getMethodStr();
	this->_env["HTTP_COOKIE"] = req.getHeader("cookie");
	this->_env["DOCUMENT_ROOT"] = location.getRootLocation();
	this->_env["REQUEST_URI"] = req.getPath() + (req.getQuery().empty() ? "" : "?" + req.getQuery());
	this->_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	this->_env["REDIRECT_STATUS"] = "200";
	this->_env["SERVER_SOFTWARE"] = "WEBSERV";

	this->_ch_env = (char **)calloc(this->_env.size() + 1 , sizeof(char *));
	std::map<std::string, std::string>::const_iterator it = this->_env.begin();
	for (int i = 0; it != this->_env.end(); it++, i++)
	{
		std::string tmp = it->first + "=" + it->second;
		this->_ch_env[i] = strdup(tmp.c_str());
	}
	this->_argv = (char **)malloc(sizeof(char *) * 3);
	this->_argv[0] = strdup(ext_path.c_str());
	this->_argv[1] = strdup(this->_cgi_path.c_str());
	this->_argv[2] = NULL;
}

// ─── non-blocking CGI lifecycle ─────────────────────────────────────────────

short CgiHandler::start(Request& request)
{
	(void)request;
	if (this->_argv == NULL || this->_ch_env == NULL || this->_argv[0] == NULL)
		return 500;
	if (pipe(pipe_in) < 0)
		return 500;
	if (pipe(pipe_out) < 0)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		return 500;
	}

	this->_cgi_pid = fork();
	if (this->_cgi_pid < 0)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		return 500;
	}
	if (this->_cgi_pid == 0)
	{
		// ── child ──
		dup2(pipe_in[0],  STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		execve(this->_argv[0], this->_argv, this->_ch_env);
		exit(1); // execve only returns on failure
	}

	// ── parent ──
	close(pipe_in[0]);   // child's read end
	close(pipe_out[1]);  // child's write end
	pipe_in[0]  = -1;
	pipe_out[1] = -1;

	// Make the parent ends non-blocking so the poll loop never stalls.
	int flags;
	flags = fcntl(pipe_in[1], F_GETFL, 0);
	if (flags >= 0)
		fcntl(pipe_in[1], F_SETFL, flags | O_NONBLOCK);
	flags = fcntl(pipe_out[0], F_GETFL, 0);
	if (flags >= 0)
		fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK);
	return 0;
}

int CgiHandler::getReadFd() const  { return this->pipe_out[0]; }
int CgiHandler::getWriteFd() const { return this->pipe_in[1]; }

void CgiHandler::closeWriteFd()
{
	if (this->pipe_in[1] >= 0)
	{
		close(this->pipe_in[1]);
		this->pipe_in[1] = -1;
	}
}

int CgiHandler::reapChild()
{
	int status = 0;
	if (this->_cgi_pid > 0)
	{
		waitpid(this->_cgi_pid, &status, 0);
		this->_cgi_pid = -1;
	}
	return status;
}

void CgiHandler::killChild()
{
	if (this->_cgi_pid > 0)
	{
		int status = 0;
		if (waitpid(this->_cgi_pid, &status, WNOHANG) == 0)
		{
			kill(this->_cgi_pid, SIGKILL);
			waitpid(this->_cgi_pid, &status, 0);
		}
		this->_cgi_pid = -1;
	}
}

// ─── utilities ──────────────────────────────────────────────────────────────

int CgiHandler::findStart(const std::string path, const std::string delim)
{
	if (path.empty())
		return (-1);
	size_t poz = path.find(delim);
	if (poz != std::string::npos)
		return (poz);
	else
		return (-1);
}

std::string CgiHandler::decode(std::string &path)
{
	size_t token = path.find("%");
	while (token != std::string::npos)
	{
		if (path.length() < token + 2)
			break ;
		char decimal = fromHexToDec(path.substr(token + 1, 2));
		path.replace(token, 3, toString(decimal));
		token = path.find("%");
	}
	return (path);
}

std::string CgiHandler::getPathInfo(std::string& path, std::vector<std::string> extensions)
{
	std::string tmp;
	size_t start, end;

	for (std::vector<std::string>::iterator it = extensions.begin(); it != extensions.end(); it++)
	{
		start = path.find(*it);
		if (start != std::string::npos)
			break ;
	}
	if (start == std::string::npos)
		return "";
	if (start + 3 >= path.size())
		return "";
	tmp = path.substr(start + 3, path.size());
	if (!tmp[0] || tmp[0] != '/')
		return "";
	end = tmp.find("?");
	return (end == std::string::npos ? tmp : tmp.substr(0, end));
}

void CgiHandler::clear()
{
	this->_cgi_pid = -1;
	this->_exit_status = 0;
	this->_cgi_path = "";
	this->_ch_env = NULL;
	this->_argv = NULL;
	this->_env.clear();
}
