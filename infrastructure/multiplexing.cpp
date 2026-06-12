# include "../includes/multiplexing.hpp"
# include "../includes/response.hpp"
# include "../includes/SessionManager.hpp"

# include <unistd.h>
# include <sys/sendfile.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <ctime>

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

static void erase_client_state(std::vector<struct pollfd>& fds_list,
								   std::map<int, client>& client_data,
								   std::map<int, size_t>& client_server_index,
								   int fd)
{
	for (size_t i = 0; i < fds_list.size(); ++i)
	{
		if (fds_list[i].fd == fd)
		{
			fds_list.erase(fds_list.begin() + i);
			break;
		}
	}
	client_server_index.erase(fd);
	client_data.erase(fd);
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

		void	multiplexing::build_and_queue_response(int fd)
		{
			std::map<int, client>::iterator client_idx = client_data.find(fd);
			if (client_idx == client_data.end())
				throw MultiplexingExcption("inavlid client");

			std::string requestPath = client_idx->second.get_request().getPath();
			const std::string& requestQuery = client_idx->second.get_request().getQuery();
			if (!requestQuery.empty())
				requestPath += "?" + requestQuery;

			Response response;
			std::map<int, size_t>::iterator server_idx = client_server_index.find(fd);
			if (server_idx != client_server_index.end())
			{
				const Request& request = client_idx->second.get_request();
				const ServerConfig* server = select_server_for_request(request, server_idx->second);
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
			const std::string& cookieHeader = client_idx->second.get_request().getHeader("cookie");
			SessionManager::Result session = _sessions.getOrCreate(cookieHeader);
			if (session.isNew)
				response.setHeader("Set-Cookie", _sessions.buildSetCookieHeader(session.id));
			
			client_idx->second.set_response_from_response(response);
			client_idx->second.reset_bytes_sent();
			set_client_as_finished(fd);
		}

		void	multiplexing::send_pending_response(int fd)
		{
			std::map<int, client>::iterator client_idx = client_data.find(fd);
			if (client_idx == client_data.end())
				return;

			const std::string& payload = client_idx->second.get_response();
			size_t header_size = payload.size();
			size_t sent_total = client_idx->second.get_bytes_sent();

			// 1) Send remaining headers/body part (if any)
			while (sent_total < header_size)
			{
				ssize_t sent = send(fd, payload.data() + sent_total, header_size - sent_total, 0);
				if (sent < 0)
				{
					if (errno == EINTR)
						continue;
					// If send fails despite POLLOUT, treat as transient: return and wait for next poll.
					return;
				}
				if (sent == 0)
					break;
				sent_total += static_cast<size_t>(sent);
				client_idx->second.add_bytes_sent(static_cast<size_t>(sent));
				client_idx->second.touch_activity();
			}

			// 2) If there is a file to stream, send it via sendfile
			if (client_idx->second.is_streaming_file())
			{
				int in_fd = client_idx->second.get_file_fd();
				off_t off = client_idx->second.get_file_offset();
				size_t remaining = client_idx->second.get_file_size() - static_cast<size_t>(off);
				while (remaining > 0)
				{
					ssize_t sf = sendfile(fd, in_fd, &off, remaining);
					if (sf > 0)
					{
						// sendfile may update 'off' when provided a pointer (Linux semantics).
						// Use the updated offset rather than incrementing again which
						// would skip data when the kernel already advanced it.
						client_idx->second.set_file_offset(off);
						client_idx->second.touch_activity();
						remaining = client_idx->second.get_file_size() - static_cast<size_t>(off);
						continue;
					}
					if (sf == 0)
						break;
					if (sf < 0)
					{
						if (errno == EINTR)
							continue;
						// Treat other errors as transient here and return to poll-driven retry.
						return;
					}
				}

				// file fully sent -> close connection
				if (static_cast<size_t>(client_idx->second.get_file_offset()) >= client_idx->second.get_file_size())
				{
					if (client_idx->second.get_file_fd() >= 0)
						close(client_idx->second.get_file_fd());
					close(fd);
					erase_client_state(fds_list, client_data, client_server_index, fd);
				}
				return;
			}

			// No file streaming: fully sent headers+body -> close connection
			if (sent_total >= header_size)
			{
				close(fd);
				erase_client_state(fds_list, client_data, client_server_index, fd);
			}
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
				throw MultiplexingExcption("inavlid client");
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
			if (rb < 0)
			{
				// Non-fatal recv error (could be transient). Rely on poll readiness rather than errno checks.
				return;
			}
			if (rb == 0)
			{
				std::cout << "the client : "<< fd << " disconnected" << std::endl;
				close (fd);
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

			std::cout << "the server is running ..." << std::endl;

			while (true)
			{
				// Avoid calling poll() with an empty array (UB). If no fds are present,
				// sleep briefly and continue.
				if (fds_list.empty())
				{
					usleep(1000); // 1ms
					continue;
				}
				// Evict idle clients before blocking in poll()
				time_t now = std::time(NULL);
				std::vector<int> to_close;
				for (std::map<int, client>::iterator it = client_data.begin(); it != client_data.end(); ++it)
				{
					if ((now - it->second.get_last_activity()) > CLIENT_IDLE_TIMEOUT)
						to_close.push_back(it->first);
				}
				for (size_t k = 0; k < to_close.size(); ++k)
				{
					int cfd = to_close[k];
					std::cerr << "Closing idle client " << cfd << " due to timeout" << std::endl;
					close(cfd);
					abort_client(cfd);
				}

				activity = poll(&fds_list[0], fds_list.size() , -1);
				if (activity < 0 )
				{
					if (errno == EINTR) // just the os pauses the code we baypass this 
						continue;
					throw  MultiplexingExcption("Global Poll Failure: " + std::string(strerror(errno)));
				}
				for (size_t i = fds_list.size(); i-- > 0; )
				{	
					if (fds_list[i].revents & POLLHUP)
					{
						std::cerr << "the client : " << fds_list[i].fd << "hung up " << std::endl;
						close(fds_list[i].fd);
						abort_client(fds_list[i].fd);
					}
					if (fds_list[i].revents & POLLERR)
					{
						// the socket encountrede a hardware or kernel error
						close(fds_list[i].fd);
						abort_client(fds_list[i].fd);
					}
					if (fds_list[i].revents & POLLIN) // an event occured ;
					{
						if (is_master_socket(fds_list[i].fd))
							add_new_client(fds_list[i].fd);
						else
							existing_client(fds_list[i].fd);
					}
					if (fds_list[i].revents & POLLOUT)
					{
						if (!is_master_socket(fds_list[i].fd))
							send_pending_response(fds_list[i].fd);
					}
				}
			}
		}

		MultiplexingExcption::MultiplexingExcption::MultiplexingExcption(const std::string& Msg) :msg("Multiplexing Error : " + Msg) {}

		const char * MultiplexingExcption::what() const throw() { return (msg.c_str()); }

		MultiplexingExcption::~MultiplexingExcption() throw() {}