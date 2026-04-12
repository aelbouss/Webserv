# include "../inc/multiplexing.hpp"


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
			fcntl(new_client, F_SETFL, O_NONBLOCK);
			client_card.fd = new_client ;
			client_card.events = POLLIN ;
			client_card.revents = 0;
			fds_list.push_back(client_card);
			client_data.insert(std::pair <int ,client>(new_client, client_room));
		}


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
		}

		void	multiplexing::existing_client(int fd)
		{
			char buffer[8000];
			int	rb;

			size_t	max_body_size  = 1000000 ; // just hardcoded
			std::map<int, client>::iterator client_idx;
		
			rb = 1;
			memset(buffer, 0, sizeof(buffer));
			while ((rb = recv(fd, buffer, sizeof(buffer) , 0)) > 0)
			{
				client_idx = client_data.find(fd);
				if (client_idx == client_data.end())
				{
					// must make exception here .
					std::cerr << "invalid client" << std::endl;
				}
				client_idx->second.append_request(buffer);
				if (client_idx->second.check_headers_is_finish())
				{
					client_idx->second.set_headers_complete();
					client_idx->second.set_content_length();
				}
				if (client_idx->second.get_request_size() > max_body_size)
				{
					std::cout << "the client exeeds the max body size" << std::endl;
					abort_client(fd);
					// throw an  exception of Error413Exception()
				}
				else
				{
					std::cout << client_idx->second.get_request() << std::endl;
				}
				memset(buffer, 0,sizeof(buffer));
				if (rb <  0)
				{
					// error to handle ;
					close(fd);
					std::cerr << "error occurs here" << std::endl;
					break ;
				}
			}
			if (rb == 0)
			{
				std::cout << "the client closes the connection" << std::endl;
				close (fd);
				abort_client(fd);
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
		 * the routine below runs  the server main job 
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
					std::cerr << "an error ocuured " << std::endl;
					exit(1);
					// thrwo exception .
				}
				for (size_t i = 0 ; i < fds_list.size() ; i++)
				{
					if (fds_list[i].revents & POLLIN) // an event occured ;
					{
						if (is_master_socket(fds_list[i].fd))
							add_new_client(fds_list[i].fd);
						else
							existing_client(fds_list[i].fd);
					}
				}

			}
		}