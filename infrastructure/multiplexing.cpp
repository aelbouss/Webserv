# include "../includes/multiplexing.hpp"
# include "../includes/response.hpp"
# include "../includes/SessionManager.hpp"

# include <unistd.h>
# include <sys/sendfile.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <ctime>
# include <csignal>

// Set by a signal handler (see main.cpp) to request a graceful shutdown so
// that destructors run and leak checkers can produce a clean report.
volatile sig_atomic_t g_stop = 0;

// Idle timeout for client connections (seconds). Close clients inactive longer than this.
static const int CLIENT_IDLE_TIMEOUT = 60;

static std::string strip_port_from_host(const std::string& host)
{
	size_t colon = host.find(':');
	if (colon == std::string::npos)
		return host;
	return host.substr(0, colon);
}

static std::string addr_to_string(in_addr_t addr)
{
	struct in_addr in;
	char ipbuf[INET_ADDRSTRLEN];

	in.s_addr = addr;
	const char *ipstr = inet_ntop(AF_INET, &in, ipbuf, sizeof(ipbuf));
	return ipstr ? std::string(ipstr) : std::string();
}

// Close a client connection cleanly. No reading of the fd is performed here
// (draining a socket outside poll() is forbidden); we simply close it.
static void close_connection(int fd)
{
	if (fd >= 0)
		close(fd);
}


		multiplexing::multiplexing() {}

		multiplexing&	multiplexing::operator = (const multiplexing& src)
		{ 
			if (this == &src)
				return (*this);	
			return *this ;
		}
		multiplexing::multiplexing( const multiplexing& src)
		{
			*this = src ;
		}

		multiplexing::~multiplexing(){}

		size_t	multiplexing::master_socket_index(int fd) const
		{
			for (size_t i = 0; i < master_sockets.size(); ++i)
			{
				if (master_sockets[i] == fd)
					return i;
			}
			return master_sockets.size();
		}

		void	multiplexing::set_server_configs(const std::vector<ServerConfig>& servers)
		{
			server_configs = servers;
		}

		void	multiplexing::set_server_groups(const std::vector< std::vector<size_t> >& groups)
		{
			server_groups = groups;
		}

const ServerConfig* multiplexing::select_server_for_request(
    const Request& request,
    size_t socket_idx
) const
{
    if (server_groups.empty())
    {
        if (socket_idx < server_configs.size())
            return &server_configs[socket_idx];
        return NULL;
    }

    if (socket_idx >= server_groups.size() || server_groups[socket_idx].empty())
        return NULL;

    const std::vector<size_t>& group = server_groups[socket_idx];
    std::string hostHeader = strip_port_from_host(request.getHeader("host"));

    if (!hostHeader.empty())
    {
        for (size_t i = 0; i < group.size(); ++i)
        {
            const ServerConfig& server = server_configs[group[i]];
            if (!server.getServerName().empty() &&
                server.getServerName() == hostHeader)
                return &server;
        }

        for (size_t i = 0; i < group.size(); ++i)
        {
            const ServerConfig& server = server_configs[group[i]];
            std::string ip = addr_to_string(server.getHost());

            if (!ip.empty() && hostHeader == ip)
                return &server;

            if (hostHeader == "localhost" && ip == "127.0.0.1")
                return &server;
        }
    }

    return &server_configs[group[0]];
}
/*
	* setup the master sockets to be ready to accept new connections
	* and  push them to the  list to be watched .
	*/
void	multiplexing::prepare_master_sockets()
{
	struct pollfd fds;

	for (size_t i = 0 ; i < master_sockets.size() ; i++)
	{
		fds.fd	= master_sockets[i];
		fds.events = POLLIN ;
		fds.revents = 0 ;
		fds_list.push_back(fds);
	}
}


void	multiplexing::set_master_sockets(server_infra& infos)
{
	master_sockets = infos.get_sockets();
}

/*
	* this routine bellow handles a  new connection
	*/

void	multiplexing::add_new_client(int fd)
{
	struct	sockaddr	 client_addr;
	struct	pollfd	client_card;
	socklen_t	addr_len;
	int	new_client;
	client client_room;

	addr_len = sizeof(client_addr);
	new_client = accept(fd, (struct sockaddr *)&client_addr, &addr_len);
	if (new_client == -1 )
	{
		// Accept failed (possibly transient); do not treat as fatal here.
		return;
	}
	if (fcntl(new_client, F_SETFL, O_NONBLOCK) < 0)
	{
		// Close the accepted socket on failure (do NOT close the listening socket)
		close(new_client);
		throw MultiplexingExcption(std::string("failed to set non blocking: ") + strerror(errno));
	}
	client_card.fd = new_client ;
	client_card.events = POLLIN ;
	client_card.revents = 0;
	fds_list.push_back(client_card);
	client_room.set_client_fd(new_client);
	client_room.touch_activity();
	client_room.set_finished_reading(false);
	client_data.insert(std::pair <int ,client>(new_client, client_room));
	client_server_index[new_client] = master_socket_index(fd);
}


/*
	* this routine below removes a client 
	*/

void	multiplexing::abort_client(int fd)
{
	std::map<int,client>::iterator it;
	std::map<int, client>::iterator tmp;

	// If this client has a CGI in flight, unregister its pipe fds from
	// the poll set first (the CgiHandler dtor will close them and reap
	// the child when the client is erased below).
	std::map<int, client>::iterator cli = client_data.find(fd);
	if (cli != client_data.end() && cli->second.cgi_is_active())
	{
		CgiHandler* cgi = cli->second.get_cgi();
		if (cgi)
		{
			unregister_pipe(cgi->getReadFd());
			if (cgi->getWriteFd() >= 0)
				unregister_pipe(cgi->getWriteFd());
		}
	}

	// remove  the  client from  the wtched  list
	for (size_t i = 0 ; i < fds_list.size() ; i++)
	{
		if (fds_list[i].fd == fd)
		{
			fds_list.erase(fds_list.begin() + i);
			break;
		}
	}
	// remove the data dat in the room of a client .
	it = client_data.begin();
	while (it != client_data.end())
	{
		if (it->first == fd)
		{
			tmp = it;
			++it;
			client_data.erase(tmp);
		}
		else
			++it;
	}
	client_server_index.erase(fd);
}

void	multiplexing::finalize_and_queue(int fd, Response& response)
{
	std::map<int, client>::iterator client_idx = client_data.find(fd);
	if (client_idx == client_data.end())
		return;
	// Attach a server session cookie if the client doesn't have one yet.
	// (Coexists with any Set-Cookie the CGI itself produced.)
	const std::string& cookieHeader = client_idx->second.get_request().getHeader("cookie");
	SessionManager::Result session = _sessions.getOrCreate(cookieHeader);
	if (session.isNew)
		response.addSetCookie(_sessions.buildSetCookieHeader(session.id));

	client_idx->second.set_response_from_response(response);
	client_idx->second.reset_bytes_sent();
	set_client_as_finished(fd);
}

void	multiplexing::build_and_queue_response(int fd)
{
	std::map<int, client>::iterator client_idx = client_data.find(fd);
	if (client_idx == client_data.end())
		return;

	std::string requestPath = client_idx->second.get_request().getPath();
	const std::string& requestQuery = client_idx->second.get_request().getQuery();
	if (!requestQuery.empty())
		requestPath += "?" + requestQuery;

	Response response;
	const ServerConfig* server = NULL;
	std::map<int, size_t>::iterator server_idx = client_server_index.find(fd);
	if (server_idx != client_server_index.end())
	{
		const Request& request = client_idx->second.get_request();
		server = select_server_for_request(request, server_idx->second);
		if (server)
		{
			const Location *location = server->matchLocation(request.getPath());
			response.build(request.getMethodStr(), requestPath,
					request.getBody(), location, server, server->getRoot(),
					&client_idx->second.get_request());
		}
		else
		{
			response.setStatus(500);
			response.setHeader("Content-Type", "text/plain");
			response.setBody("Internal Server Error");
		}
	}
	else
	{
		response.setStatus(500);
		response.setHeader("Content-Type", "text/plain");
		response.setBody("Internal Server Error");
	}

	// If the response routed to a CGI script, the child has already been
	// forked with non-blocking pipes. Hand it off to the poll loop instead
	// of building/sending a response now.
	if (response.isCgiPending())
	{
		client_idx->second.attach_cgi(response.detachCgi(),
										response.getCgiInput(), server);
		register_cgi(fd);
		return;
	}

	finalize_and_queue(fd, response);
}

void	multiplexing::send_pending_response(int fd)
{
	std::map<int, client>::iterator client_idx = client_data.find(fd);
	if (client_idx == client_data.end())
		return;

	const std::string& payload = client_idx->second.get_response();
	size_t total = payload.size();
	size_t sent_total = client_idx->second.get_bytes_sent();

	if (sent_total < total)
	{
		// Exactly ONE send() per POLLOUT event. The poll loop will call
		// us again on the next iteration if there is more to send.
		ssize_t sent = send(fd, payload.data() + sent_total,
								total - sent_total, 0);
		if (sent > 0)
		{
			client_idx->second.add_bytes_sent(static_cast<size_t>(sent));
			client_idx->second.touch_activity();
			if (client_idx->second.get_bytes_sent() >= total)
			{
				// Whole response delivered -> close (Connection: close).
				close_connection(fd);
				abort_client(fd);
			}
			return;
		}
		// sent <= 0 : peer closed (0) or write error (<0). In either
		// case the client is removed (no errno inspection).
		close_connection(fd);
		abort_client(fd);
		return;
	}

	// Nothing left to send.
	close_connection(fd);
	abort_client(fd);
}


// ── async CGI: register pipe fds in the poll set ──
void	multiplexing::set_client_events(int fd, short events)
{
	for (size_t i = 0; i < fds_list.size(); ++i)
	{
		if (fds_list[i].fd == fd)
		{
			fds_list[i].events = events;
			break;
		}
	}
}

void	multiplexing::unregister_pipe(int pipe_fd)
{
	if (pipe_fd < 0)
		return;
	for (size_t i = 0; i < fds_list.size(); ++i)
	{
		if (fds_list[i].fd == pipe_fd)
		{
			fds_list.erase(fds_list.begin() + i);
			break;
		}
	}
	cgi_pipe_to_client.erase(pipe_fd);
}

void	multiplexing::register_cgi(int client_fd)
{
	std::map<int, client>::iterator it = client_data.find(client_fd);
	if (it == client_data.end())
		return;
	CgiHandler* cgi = it->second.get_cgi();
	if (!cgi)
		return;

	int read_fd  = cgi->getReadFd();
	int write_fd = cgi->getWriteFd();

	// Stop watching the client socket while the CGI runs (we have nothing
	// to read from it and nothing to send yet).
	set_client_events(client_fd, 0);

	// Always watch the read end for CGI output.
	struct pollfd rfd;
	rfd.fd = read_fd;
	rfd.events = POLLIN;
	rfd.revents = 0;
	fds_list.push_back(rfd);
	cgi_pipe_to_client[read_fd] = client_fd;

	// Watch the write end only if there is a POST body to feed; otherwise
	// close it now so the CGI sees EOF on stdin immediately.
	if (it->second.cgi_has_input())
	{
		struct pollfd wfd;
		wfd.fd = write_fd;
		wfd.events = POLLOUT;
		wfd.revents = 0;
		fds_list.push_back(wfd);
		cgi_pipe_to_client[write_fd] = client_fd;
	}
	else
		cgi->closeWriteFd();
}

void	multiplexing::handle_cgi_write(int pipe_fd)
{
	std::map<int, int>::iterator m = cgi_pipe_to_client.find(pipe_fd);
	if (m == cgi_pipe_to_client.end())
		return;
	std::map<int, client>::iterator it = client_data.find(m->second);
	if (it == client_data.end())
	{
		unregister_pipe(pipe_fd);
		return;
	}
	CgiHandler* cgi = it->second.get_cgi();
	const std::string& in = it->second.get_cgi_input();
	size_t off = it->second.get_cgi_in_off();

	if (off < in.size())
	{
		// Exactly ONE write() per POLLOUT event.
		ssize_t w = write(pipe_fd, in.data() + off, in.size() - off);
		if (w > 0)
		{
			it->second.add_cgi_in_off(static_cast<size_t>(w));
			if (it->second.get_cgi_in_off() >= in.size())
			{
				unregister_pipe(pipe_fd);
				if (cgi)
					cgi->closeWriteFd();   // EOF to CGI stdin
			}
			return;
		}
		// w <= 0 : stop feeding; close the write end (no errno inspection).
		unregister_pipe(pipe_fd);
		if (cgi)
			cgi->closeWriteFd();
		return;
	}
	// Nothing left to write.
	unregister_pipe(pipe_fd);
	if (cgi)
		cgi->closeWriteFd();
}

void	multiplexing::handle_cgi_read(int pipe_fd)
{
	std::map<int, int>::iterator m = cgi_pipe_to_client.find(pipe_fd);
	if (m == cgi_pipe_to_client.end())
		return;
	int client_fd = m->second;
	std::map<int, client>::iterator it = client_data.find(client_fd);
	if (it == client_data.end())
	{
		unregister_pipe(pipe_fd);
		return;
	}

	char buf[8192];
	// Exactly ONE read() per POLLIN event.
	ssize_t r = read(pipe_fd, buf, sizeof(buf));
	if (r > 0)
	{
		it->second.append_cgi_out(buf, static_cast<size_t>(r));
		it->second.touch_activity();
		return;
	}
	// r == 0 (EOF, CGI done) or r < 0 (error): finalize either way.
	finish_cgi(client_fd, false);
}

void	multiplexing::finish_cgi(int client_fd, bool timed_out)
{
	std::map<int, client>::iterator it = client_data.find(client_fd);
	if (it == client_data.end())
		return;
	CgiHandler* cgi = it->second.get_cgi();
	if (!cgi)
		return;

	// Detach pipe fds from the poll set before the handler closes them.
	unregister_pipe(cgi->getReadFd());
	if (cgi->getWriteFd() >= 0)
		unregister_pipe(cgi->getWriteFd());

	// Reap the child (bounded: WNOHANG, then SIGKILL + wait if needed).
	cgi->killChild();

	Response response;
	if (timed_out)
		response.cgiErrorPage(504, it->second.get_cgi_server());
	else
		response.applyCgiOutput(it->second.get_cgi_out());

	// Release the CGI handler (closes pipe ends; child already reaped).
	it->second.clear_cgi();

	// Queue the response (finalize_and_queue re-enables POLLOUT).
	finalize_and_queue(client_fd, response);
}

void	multiplexing::set_client_as_finished(int fd)
{
	for (size_t i = 0 ; i < fds_list.size() ; i++)
	{
		if (fds_list[i].fd == fd)
		{
			fds_list[i].events = POLLOUT;
			break ;
		}

	}
}

/*
	* the routine  below  handles  an existing connection
	*/

void	multiplexing::existing_client(int fd)
{
	char buffer[8192];
	int	rb;
	std::map<int, client>::iterator client_idx;

	client_idx = client_data.find(fd);
	if (client_idx == client_data.end())
		return;
	if (client_idx->second.upload_in_progress())
	{
		if (!client_idx->second.flush_upload_pending())
			return;
		if (client_idx->second.get_upload_error_code() != 0)
		{
			client_idx->second.get_request().setErrorCode(client_idx->second.get_upload_error_code());
			build_and_queue_response(fd);
			return;
		}
	}
	rb = 1;
	memset(buffer, 0, sizeof(buffer));
	rb = recv(fd, buffer, sizeof(buffer) , 0);

	if (rb > 0)
	{
		Request& req = client_idx->second.get_request();
		if (req.getRemoteAddr().empty())
		{
			struct sockaddr_in addr;
			socklen_t len = sizeof(addr);
			if (getpeername(fd, (struct sockaddr *)&addr, &len) == 0)
			{
				char ipbuf[INET_ADDRSTRLEN];
				const char *ip = inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf));
				if (ip)
					req.setRemoteAddr(ip);
			}
		}

		client_idx->second.parse_request(buffer, rb);
		client_idx->second.touch_activity();
		if (req.isHeadersComplete() && !req.isBodyConfigReady())
		{
			const ServerConfig* server = NULL;
			const Location* location = NULL;
			std::map<int, size_t>::iterator server_idx = client_server_index.find(fd);
			if (server_idx != client_server_index.end() && server_idx->second < server_configs.size())
			{
				server = select_server_for_request(req, server_idx->second);
				if (server)
					location = server->matchLocation(req.getPath());
			}

			unsigned long maxBody = (server ? server->getClientMaxBodySize() : 0);
			if (location && location->getMaxBodySize() > 0)
				maxBody = location->getMaxBodySize();
			req.setMaxBodySize(maxBody);

			bool canUpload = false;
			if (location && !location->getUploadStore().empty() && req.getMethodStr() == "POST")
			{
				std::string path = req.getPath();
				bool isCgi = false;
				if (location->getPath() == "/cgi-bin")
					isCgi = true;
				if (path.size() >= 3 && (path.substr(path.size() - 3) == ".py" || path.substr(path.size() - 3) == ".sh"))
					isCgi = true;
				if (!isCgi)
					canUpload = true;
			}

			if (canUpload)
			{
				const std::string& contentType = req.getHeader("content-type");
				if (!client_idx->second.begin_upload(location->getUploadStore(), req.getPath(), contentType, maxBody))
					req.setErrorCode(client_idx->second.get_upload_error_code() ? client_idx->second.get_upload_error_code() : 500);
				else
					req.setBodySink(&client_idx->second);
			}

			req.markBodyConfigReady();
			req.parse("", 0);
		}

		if (client_idx->second.is_parsing_finished())
		{
			client_idx->second.set_finished_reading(true);
			build_and_queue_response(fd);
			return ;
		}
	}
	if (rb <= 0)
	{
		// rb == 0 : peer disconnected.
		// rb <  0 : read error on a socket that poll() reported readable.
		// Either way the client is removed (no errno inspection).
		close_connection(fd);
		abort_client(fd);
		return ;
	}
}

/*
	* this routine below signals if the new event occured is a new connection or existing one
	*/

int	multiplexing::is_master_socket(int fd)
{
	for (size_t i = 0 ; i < master_sockets.size() ; i++)
	{
		if (fd == master_sockets[i])
			return (1);	
	}
	return (0);
}

/*
	* the routine below runs the server main job 
	*/

void	multiplexing::cluster_controlling()
{
	int	activity;
	const int CGI_TIMEOUT = 10;

	std::cout << "the server is running ..." << std::endl;

	while (g_stop == 0)
	{
		// Avoid calling poll() with an empty array (UB). If no fds are present,
		// sleep briefly and continue.
		if (fds_list.empty())
		{
			usleep(1000); // 1ms
			continue;
		}
		// Evict idle clients and time out stuck CGIs before blocking in poll()
		time_t now = std::time(NULL);
		std::vector<int> to_close;
		std::vector<int> cgi_timeouts;
		for (std::map<int, client>::iterator it = client_data.begin(); it != client_data.end(); ++it)
		{
			if (it->second.cgi_is_active())
			{
				if ((now - it->second.get_cgi_start()) > CGI_TIMEOUT)
					cgi_timeouts.push_back(it->first);
			}
			else if ((now - it->second.get_last_activity()) > CLIENT_IDLE_TIMEOUT)
				to_close.push_back(it->first);
		}
		for (size_t k = 0; k < cgi_timeouts.size(); ++k)
			finish_cgi(cgi_timeouts[k], true);
		for (size_t k = 0; k < to_close.size(); ++k)
		{
			int cfd = to_close[k];
			close_connection(cfd);
			abort_client(cfd);
		}

		if (fds_list.empty())
			continue;

		activity = poll(&fds_list[0], fds_list.size() , -1);
		if (activity < 0 )
		{
			if (errno == EINTR) // just the os pauses the code we baypass this
				continue;
			throw  MultiplexingExcption("Global Poll Failure: " + std::string(strerror(errno)));
		}

		// Snapshot the ready (fd, revents) pairs first. Handlers below may
		// erase/add entries in fds_list, so we never index it while mutating.
		std::vector<struct pollfd> ready;
		for (size_t i = 0; i < fds_list.size(); ++i)
		{
			if (fds_list[i].revents != 0)
				ready.push_back(fds_list[i]);
		}

		for (size_t i = 0; i < ready.size(); ++i)
		{
			int fd = ready[i].fd;
			short revents = ready[i].revents;
			std::map<int, int>::iterator pit = cgi_pipe_to_client.find(fd);
			if (pit != cgi_pipe_to_client.end())
			{
				int cfd = pit->second;
				std::map<int, client>::iterator cit = client_data.find(cfd);
				if (cit == client_data.end())
				{
					unregister_pipe(fd);
					continue;
				}
				CgiHandler* cgi = cit->second.get_cgi();
				bool isRead = (cgi && fd == cgi->getReadFd());
				if (isRead)
				{
					// A read end may report POLLIN and POLLHUP together when
					// the CGI writes its output and exits at once. Always read:
					// read() drains the buffered data first and only returns 0
					// (EOF) once it is fully consumed, which triggers finish.
					if (revents & (POLLIN | POLLHUP | POLLERR))
						handle_cgi_read(fd);
				}
				else
				{
					if (revents & (POLLHUP | POLLERR))
					{
						unregister_pipe(fd);
						if (cgi)
							cgi->closeWriteFd();
					}
					else if (revents & POLLOUT)
						handle_cgi_write(fd);
				}
				continue;
			}

			// 2) Master listening socket?
			if (is_master_socket(fd))
			{
				if (revents & POLLIN)
					add_new_client(fd);
				continue;
			}

			// 3) Client socket.
			if (revents & (POLLHUP | POLLERR))
			{
				close_connection(fd);
				abort_client(fd);
				continue;
			}
			if (client_data.find(fd) == client_data.end())
				continue;
			if (revents & POLLIN)
				existing_client(fd);
			if (client_data.find(fd) == client_data.end())
				continue;
			if (revents & POLLOUT)
				send_pending_response(fd);
		}
	}

	// Graceful shutdown: close every watched fd and drop all clients
	// (client destructors reap any in-flight CGI children).
	for (size_t i = 0; i < fds_list.size(); ++i)
	{
		if (fds_list[i].fd >= 0)
			close(fds_list[i].fd);
	}
	fds_list.clear();
	cgi_pipe_to_client.clear();
	client_data.clear();
	client_server_index.clear();
}

MultiplexingExcption::MultiplexingExcption::MultiplexingExcption(const std::string& Msg) :msg("Multiplexing Error : " + Msg) {}

const char * MultiplexingExcption::what() const throw() { return (msg.c_str()); }

MultiplexingExcption::~MultiplexingExcption() throw() {}