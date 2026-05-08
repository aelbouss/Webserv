#include "../includes/CgiHandler.hpp"
#include "../includes/request.hpp"
#include <sys/wait.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <ctime>


CgiHandler::CgiHandler() {
	this->_cgi_pid = -1;
	this->_exit_status = 0;
	this->_cgi_path = "";
	this->_ch_env = NULL;
	this->_argv = NULL;
}

CgiHandler::CgiHandler(std::string path)
{
	this->_cgi_pid = -1;
	this->_exit_status = 0;
	this->_cgi_path = path;
	this->_ch_env = NULL;
	this->_argv = NULL;
}

CgiHandler::~CgiHandler() {

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
		this->_ch_env = other._ch_env;
		this->_argv = other._argv;
		this->_cgi_path = other._cgi_path;
		this->_cgi_pid = other._cgi_pid;
		this->_exit_status = other._exit_status;
}

CgiHandler &CgiHandler::operator=(const CgiHandler &rhs)
{
    if (this != &rhs)
	{
		this->_env = rhs._env;
		this->_ch_env = rhs._ch_env;
		this->_argv = rhs._argv;
		this->_cgi_path = rhs._cgi_path;
		this->_cgi_pid = rhs._cgi_pid;
		this->_exit_status = rhs._exit_status;
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
		std::string name = it->first;
		std::transform(name.begin(), name.end(), name.begin(), ::toupper);
		std::string key = "HTTP_" + name;
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

// Build CGI env/argv from a location's extension mapping.
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
	this->_env["REMOTE_ADDR"] = req.getHeader("host");
	int poz = findStart(req.getHeader("host"), ":");
	this->_env["SERVER_NAME"] = (poz > 0 ? req.getHeader("host").substr(0, poz) : "");
	this->_env["SERVER_PORT"] = (poz > 0 ? req.getHeader("host").substr(poz + 1, req.getHeader("host").size()) : "");
	this->_env["REQUEST_METHOD"] = req.getMethodStr();
	this->_env["HTTP_COOKIE"] = req.getHeader("cookie");
	this->_env["DOCUMENT_ROOT"] = location.getRootLocation();
	this->_env["REQUEST_URI"] = req.getPath() + req.getQuery();
	this->_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	this->_env["REDIRECT_STATUS"] = "200";
	this->_env["SERVER_SOFTWARE"] = "AMANIX";

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

void CgiHandler::initEnvCgi(Request& req, const std::vector<Location>::iterator it_loc)
{
	std::string cgi_exec = ("cgi-bin/" + it_loc->getCgiPath()[0]).c_str();
	char    *cwd = getcwd(NULL, 0);
	if(_cgi_path[0] != '/')
	{
		std::string tmp(cwd);
		tmp.append("/");
		if(_cgi_path.length() > 0)
			_cgi_path.insert(0, tmp);
	}
	if(req.getMethod() == Request::POST)
	{
		std::stringstream out;
		out << req.getBody().size();
		this->_env["CONTENT_LENGTH"] = out.str();
		this->_env["CONTENT_TYPE"] = req.getHeader("content-type");
	}

    this->_env["GATEWAY_INTERFACE"] = std::string("CGI/1.1");
	this->_env["SCRIPT_NAME"] = cgi_exec;//
    this->_env["SCRIPT_FILENAME"] = this->_cgi_path;
    this->_env["PATH_INFO"] = this->_cgi_path;//
    this->_env["PATH_TRANSLATED"] = this->_cgi_path;//
    this->_env["REQUEST_URI"] = this->_cgi_path;//
    this->_env["SERVER_NAME"] = req.getHeader("host");
    this->_env["SERVER_PORT"] ="8002";
	this->_env["REQUEST_METHOD"] = req.getMethodStr();
    this->_env["SERVER_PROTOCOL"] = "HTTP/1.1";
    this->_env["REDIRECT_STATUS"] = "200";
	this->_env["SERVER_SOFTWARE"] = "AMANIX";

	std::map<std::string, std::string> request_headers = req.getHeaders();
	for(std::map<std::string, std::string>::iterator it = request_headers.begin();
		it != request_headers.end(); ++it)
	{
		std::string name = it->first;
		std::transform(name.begin(), name.end(), name.begin(), ::toupper);
		std::string key = "HTTP_" + name;
		_env[key] = it->second;
	}
	this->_ch_env = (char **)calloc(this->_env.size() + 1, sizeof(char *));
	std::map<std::string, std::string>::const_iterator it = this->_env.begin();
	for (int i = 0; it != this->_env.end(); it++, i++)
	{
		std::string tmp = it->first + "=" + it->second;
		this->_ch_env[i] = strdup(tmp.c_str());
	}
	this->_argv = (char **)malloc(sizeof(char *) * 3);
	this->_argv[0] = strdup(cgi_exec.c_str());
	this->_argv[1] = strdup(this->_cgi_path.c_str());
	this->_argv[2] = NULL;
}


void CgiHandler::initEnv(Request& req, const std::vector<Location>::iterator it_loc)
{
	int			poz;
	std::string extension;
	std::string ext_path;

	extension = this->_cgi_path.substr(this->_cgi_path.find("."));
	std::map<std::string, std::string>::iterator it_path = it_loc->_ext_path.find(extension);
    if (it_path == it_loc->_ext_path.end())
        return ;
    ext_path = it_loc->_ext_path[extension];

	this->_env["AUTH_TYPE"] = "Basic";
	this->_env["CONTENT_LENGTH"] = req.getHeader("content-length");
	this->_env["CONTENT_TYPE"] = req.getHeader("content-type");
    this->_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	poz = findStart(this->_cgi_path, "cgi-bin/");
	this->_env["SCRIPT_NAME"] = this->_cgi_path;
    this->_env["SCRIPT_FILENAME"] = ((poz < 0 || (size_t)(poz + 8) > this->_cgi_path.size()) ? "" : this->_cgi_path.substr(poz + 8, this->_cgi_path.size())); // check dif cases after put right parametr from the response
	std::string path = req.getPath();
	this->_env["PATH_INFO"] = getPathInfo(path, it_loc->getCgiExtension());
    this->_env["PATH_TRANSLATED"] = it_loc->getRootLocation() + (this->_env["PATH_INFO"] == "" ? "/" : this->_env["PATH_INFO"]);
	std::string query = req.getQuery();
	this->_env["QUERY_STRING"] = decode(query);
	this->_env["REMOTE_ADDR"] = req.getHeader("host");
	poz = findStart(req.getHeader("host"), ":");
	this->_env["SERVER_NAME"] = (poz > 0 ? req.getHeader("host").substr(0, poz) : "");
	this->_env["SERVER_PORT"] = (poz > 0 ? req.getHeader("host").substr(poz + 1, req.getHeader("host").size()) : "");
    this->_env["REQUEST_METHOD"] = req.getMethodStr();
    this->_env["HTTP_COOKIE"] = req.getHeader("cookie");
    this->_env["DOCUMENT_ROOT"] = it_loc->getRootLocation();
	this->_env["REQUEST_URI"] = req.getPath() + req.getQuery();
    this->_env["SERVER_PROTOCOL"] = "HTTP/1.1";
    this->_env["REDIRECT_STATUS"] = "200";
	this->_env["SERVER_SOFTWARE"] = "AMANIX";

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

std::string CgiHandler::execute(Request& request, short &error_code)
{
	// Guard CGI execution with a fixed timeout to avoid blocking the server loop.
	const int timeoutSeconds = 5;
	const int pollSliceMs = 100;

	if (this->_argv[0] == NULL || this->_argv[1] == NULL)
	{
		error_code = 500;
		return std::string();
	}
	if (pipe(pipe_in) < 0)
	{
		error_code = 500;
		return std::string();
	}
	if (pipe(pipe_out) < 0)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		error_code = 500;
		return std::string();
	}
	this->_cgi_pid = fork();
	if (this->_cgi_pid == 0)
	{
		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		this->_exit_status = execve(this->_argv[0], this->_argv, this->_ch_env);
		exit(this->_exit_status);
	}
	else if (this->_cgi_pid > 0)
	{
		/* parent: close the child-side pipe ends that the parent doesn't use.
		   Parent will use pipe_in[1] to write (stdin for child) and
		   pipe_out[0] to read (stdout from child). Closing the other ends
		   here prevents the parent from accidentally keeping write-ends
		   open and thus avoids read() from blocking waiting for EOF. */
		close(pipe_in[0]);
		close(pipe_out[1]);

		int write_fd = this->pipe_in[1];
		int read_fd  = this->pipe_out[0];

		int flags = fcntl(read_fd, F_GETFL, 0);
		if (flags >= 0)
			fcntl(read_fd, F_SETFL, flags | O_NONBLOCK);

		// 1) Send POST body (if any), then close write end to signal EOF
		if (request.getMethod() == Request::POST)
		{
			const std::vector<char>& body_vec = request.getBody();
			size_t total = 0;
			size_t len = body_vec.size();
			while (total < len)
			{
				ssize_t w = write(write_fd, &body_vec[total], len - total);
				if (w < 0)
				{
					if (errno == EINTR) continue;
					// write error: cleanup and return
					close(write_fd);
					close(read_fd);
					error_code = 500;
					int status = 0;
					if (this->_cgi_pid > 0)
						waitpid(this->_cgi_pid, &status, 0);
					return std::string();
				}
				total += (size_t)w;
			}
		}
		// always close write end after sending (even if zero-length)
		close(write_fd);

		// 2) Read CGI output with timeout using poll()
		std::string raw;
		char buf[4096];
		time_t start = std::time(NULL);
		bool timed_out = false;
		while (true)
		{
			if (std::time(NULL) - start >= timeoutSeconds)
			{
				timed_out = true;
				break;
			}

			struct pollfd pfd;
			pfd.fd = read_fd;
			pfd.events = POLLIN | POLLHUP;
			pfd.revents = 0;
			int pr = poll(&pfd, 1, pollSliceMs);
			if (pr < 0)
			{
				if (errno == EINTR)
					continue;
				break;
			}
			if (pr == 0)
				continue;

			if (pfd.revents & (POLLIN | POLLHUP))
			{
				ssize_t r = read(read_fd, buf, sizeof(buf));
				if (r > 0)
				{
					raw.append(buf, (size_t)r);
				}
				else if (r == 0)
				{
					break;
				}
				else if (errno != EAGAIN && errno != EWOULDBLOCK)
				{
					break;
				}
			}
		}
		close(read_fd);

		// 3) Reap child (timeout-aware)
		int status = 0;
		pid_t pid = this->getCgiPid();
		if (pid > 0)
		{
			if (timed_out)
			{
				kill(pid, SIGKILL);
				waitpid(pid, &status, 0);
				error_code = 504;
				return std::string();
			}
			waitpid(pid, &status, 0);
		}

		return raw;
	}
	else
	{
		std::cout << "Fork failed" << std::endl;
		error_code = 500;
		return std::string();
	}
}



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

void		CgiHandler::clear()
{
	this->_cgi_pid = -1;
	this->_exit_status = 0;
	this->_cgi_path = "";
	this->_ch_env = NULL;
	this->_argv = NULL;
	this->_env.clear();
}