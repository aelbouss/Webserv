# ifndef MULTIPLEXING_HPP
# define MULTIPLEXING_HPP

# include "server_infra.hpp"
# include "client.hpp"
# include "request.hpp"
# include "ServerConfig.hpp"


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

# define MAX_BODY_SIZE = 1000;

class	multiplexing
{
	private :
		std::vector <struct pollfd> fds_list;
		std::vector <int> master_sockets ;
		std::vector <ServerConfig> server_configs;
		std::map <int, client> client_data; // to just distinguish master vs client sockets .
		std::map <int, size_t> client_server_index;
		size_t	master_socket_index(int fd) const;
		void	build_and_queue_response(int fd);
		void	send_pending_response(int fd);
	public :
		multiplexing();
		multiplexing&	operator = (const multiplexing& src);
		multiplexing( const multiplexing& src);
		~multiplexing();
		void	set_master_sockets(server_infra& infos);
		void	set_server_configs(const std::vector<ServerConfig>& servers);
		void	add_new_client(int fd);
		void	remove_client(int fd);
		void	take_master_sockets_snapshot(server_infra& infos);
		void	prepare_master_sockets();
		
		void	cluster_controlling();	
		int	is_master_socket(int fd);
		void existing_client(int fd);
		void abort_client(int fd);
		void	set_client_as_finished(int fd);
};

# endif