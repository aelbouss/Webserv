# ifndef CLIENT_HPP
# define CLIENT_HPP

# include "server_infra.hpp"
# include "request.hpp"


class	client
{

	private:
		int	fd;
		Request	request;
		bool	is_finished_reading;
		size_t	bytes_sent;
	public:

		client();
		client&	operator = (const client& src);
		client(const client& src);
		~client();
		void	set_client_fd(int client_fd);
		int		get_client_id();
		void	set_finished_reading(bool var);
		bool	get_finished_reading() ;
		void	parse_request(char *data, int rb);
		bool	is_parsing_finished();
};

# endif