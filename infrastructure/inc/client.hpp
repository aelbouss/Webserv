# ifndef CLIENT_HPP
# define CLIENT_HPP

# include "server_infra.hpp"


class	client
{
	private:
		int client_id; // the client socket 
		std::string request_buffer; // string to store request ;
		bool	headers_complete;
		size_t	content_length; // tells  you how many bytes  in the body 
		size_t	header_size; // to know tha start point of the request body  
	public:
		client();
		client&	operator = (const client& src);
		client(const client& src);
		~client();
		int	get_client_id();
		std::string get_request();
		void	set_id(int fd);
		void	append_request(char *raw_bytes);
		void	set_content_length();
		void	set_header_size(size_t nb);
		void	set_headers_complete();
		bool	get_headers_complete();
		size_t	get_request_size();
		int check_headers_is_finish();
		size_t	get_content_length();
		size_t	get_header_size();

};

# endif