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
		
			addr_len = sizeof(client_addr);
			new_client = accept(fd, (struct sockaddr *)&client_addr, &addr_len);
			fcntl(new_client, F_SETFL, O_NONBLOCK);
			client_card.fd = new_client ;
			client_card.events = POLLIN ;
			client_card.revents = 0;
			fds_list.push_back(client_card);
		}

		// void	multiplexing::remove_client(int fd)
		// {

		// }


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

		void	multiplexing::cluster_controlling()
		{
			int	activity;

			while (true)
			{
				activity = poll(&fds_list[0], fds_list.size() , -1);
				if (activity < 0 )
				{
					// thrwo exception .
				}

				for (size_t i = 0 ; i < fds_list.size() ; i++)
				{
					if (fds_list[i].revents & POLLIN) // an event occured ;
					{
						if (is_master_socket(fds_list[i].fd))
						{
							add_new_client(fds_list[i].fd);
						}
						else
						{
							// handle  an  existing  client .
						}
					}
				}

			}
		}