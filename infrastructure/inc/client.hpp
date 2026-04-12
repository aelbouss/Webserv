# ifndef CLIENT_HPP
# define CLIENT_HPP

# include "server_infra.hpp"


class	client
{
	private:
		int client_id; // the client socket 
		std::string request_buffer; // string to store request ;
	public:
		client();
		client&	operator = (const client& src);
		client(const client& src);
		~client();
		int	get_client_id();
		std::string get_request();
		void	set_id(int fd);
		void	append_request(char *raw_bytes);
};

# endif