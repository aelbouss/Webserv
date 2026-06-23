# ifndef MULTIPLEXING_HPP
# define MULTIPLEXING_HPP

# include "server_infra.hpp"
# include "client.hpp"
# include "request.hpp"
# include "ServerConfig.hpp"
# include "SessionManager.hpp"


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
class	Response;

class	multiplexing
{
	private :
		std::vector <struct pollfd> fds_list;
		std::vector <int> master_sockets ;
		std::vector <ServerConfig> server_configs;
		std::vector< std::vector<size_t> > server_groups;
		std::map <int, client> client_data; // to just distinguish master vs client sockets .
		std::map <int, size_t> client_server_index;
		// Maps each live CGI pipe fd (read or write end) to its owning client fd.
		std::map <int, int> cgi_pipe_to_client;
		SessionManager _sessions;
		size_t	master_socket_index(int fd) const;
		const ServerConfig* select_server_for_request(
			const Request& request,
			size_t socket_idx
		) const;
		void	build_and_queue_response(int fd);
		void	send_pending_response(int fd);
		// ── async CGI helpers (all I/O driven by the main poll loop) ──
		void	register_cgi(int client_fd);
		void	handle_cgi_read(int pipe_fd);
		void	handle_cgi_write(int pipe_fd);
		void	finish_cgi(int client_fd, bool timed_out);
		void	unregister_pipe(int pipe_fd);
		void	set_client_events(int fd, short events);
		void	finalize_and_queue(int fd, Response& response);
	public :
		multiplexing();
		multiplexing&	operator = (const multiplexing& src);
		multiplexing( const multiplexing& src);
		~multiplexing();
		void	set_master_sockets(server_infra& infos);
		void	set_server_configs(const std::vector<ServerConfig>& servers);
		void	set_server_groups(const std::vector< std::vector<size_t> >& groups);
		void	add_new_client(int fd);
		void	remove_client(int fd);
		void	take_master_sockets_snapshot(server_infra& infos);
		void	prepare_master_sockets();
		
		void	cluster_controlling();	
		bool handle_cgi_pipe_event(int fd, short revents);
		int	is_master_socket(int fd);
		void existing_client(int fd);
		void abort_client(int fd);
		void	set_client_as_finished(int fd);
};

# endif