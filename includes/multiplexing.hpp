# ifndef MULTIPLEXING_HPP
# define MULTIPLEXING_HPP

# include "server_infra.hpp"
# include "client.hpp"
# include "request.hpp"


class	MultiplexingExcption : public std::exception
{
	private:
		std::string	msg;

	public:
		MultiplexingExcption(const std::string& Msg);
		virtual	const char * what() const throw() ;
		virtual	~MultiplexingExcption() throw() ;
};

class	server_infra;
class	client;
class	Request;
class	ServerConfig;

# define MAX_BODY_SIZE = 1000;

class	multiplexing
{
	private :
		std::vector <struct pollfd> fds_list;
		std::vector <int> master_sockets ;
		std::map <int, client> client_data; // to just distinguish master vs client sockets .
		// Server config pointers for routing by host/port.
		std::vector <const ServerConfig*> servers;
	public :
		multiplexing();
		multiplexing&	operator = (const multiplexing& src);
		multiplexing( const multiplexing& src);
		~multiplexing();
		void	set_master_sockets(server_infra& infos);
		// Provide parsed servers to the network loop for routing.
		void	set_servers(const std::vector<ServerConfig>& servers_list);
		void	add_new_client(int fd);
		void	remove_client(int fd);
		void	take_master_sockets_snapshot(server_infra& infos);
		void	prepare_master_sockets();
		
		void	cluster_controlling();	
		int	is_master_socket(int fd);
		void existing_client(int fd);
		void abort_client(int fd);
		void	set_client_as_finished(int fd);
		// Flush buffered response data to the socket.
		void	send_response(int fd);
};

# endif