# include "../includes/multiplexing.hpp"
# include "../includes/response.hpp"

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
			struct	pollfd		client_card;
			socklen_t	addr_len;
			int	new_client;
			client client_room;
		
			addr_len = sizeof(client_addr);
			new_client = accept(fd, (struct sockaddr *)&client_addr, &addr_len);
			if (new_client == -1 )
				throw MultiplexingExcption("failed to add new client");
			if (fcntl(new_client, F_SETFL, O_NONBLOCK) < 0)
			{
				close(fd);
				throw MultiplexingExcption("failed to set non blocking");
			}
			client_card.fd = new_client ;
			client_card.events = POLLIN ;
			client_card.revents = 0;
			fds_list.push_back(client_card);
			client_room.set_client_fd(new_client);
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
			if (server_idx != client_server_index.end() && server_idx->second < server_configs.size())
			{
				ServerConfig& server = server_configs[server_idx->second];
				const Location *location = server.matchLocation(client_idx->second.get_request().getPath());
				response.build(client_idx->second.get_request().getMethodStr(), requestPath, 
							   client_idx->second.get_request().getBody(), location, &server);
			}
			else
			{
				response.setStatus(500);
				response.setHeader("Content-Type", "text/plain");
				response.setBody("Internal Server Error");
			}
			client_idx->second.set_response(response.toString());
			client_idx->second.reset_bytes_sent();
			set_client_as_finished(fd);
		}

		void	multiplexing::send_pending_response(int fd)
		{
			std::map<int, client>::iterator client_idx = client_data.find(fd);
			if (client_idx == client_data.end())
				return;

			const std::string& payload = client_idx->second.get_response();
			size_t sent_total = client_idx->second.get_bytes_sent();

			while (sent_total < payload.size())
			{
				ssize_t sent = send(fd, payload.data() + sent_total, payload.size() - sent_total, 0);
				if (sent < 0)
				{
					if (errno == EINTR)
						continue;
					if (errno == EAGAIN || errno == EWOULDBLOCK)
						return;
					std::cerr << "Send error on fd " << fd << ": " << strerror(errno) << std::endl;
					close(fd);
					erase_client_state(fds_list, client_data, client_server_index, fd);
					return;
				}
				if (sent == 0)
					break;
				sent_total += static_cast<size_t>(sent);
				client_idx->second.add_bytes_sent(static_cast<size_t>(sent));
			}

			if (sent_total >= payload.size())
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
			rb = 1;
			memset(buffer, 0, sizeof(buffer));
			rb = recv(fd, buffer, sizeof(buffer) , 0);

			if (rb > 0)
			{
				client_idx->second.parse_request(buffer, rb);
				if (client_idx->second.is_parsing_finished())
				{
					client_idx->second.set_finished_reading(true);
						build_and_queue_response(fd);
					return ;
				}
			}
			if (rb < 0)
			{
				if (errno != EAGAIN && errno != EWOULDBLOCK)
				{
					std::cerr << "Recv error on fd " << fd << ": " << strerror(errno) << std::endl;
					close(fd);
					abort_client(fd);
					return ;
				}
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
				activity = poll(&fds_list[0], fds_list.size() , -1);
				if (activity < 0 )
				{
					if (errno == EINTR) // just the os pauses the code we baypass this 
						continue;
					throw  MultiplexingExcption("Global Poll Failure: " + std::string(strerror(errno)));
				}
				for (size_t i = 0 ; i < fds_list.size() ; i++)
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