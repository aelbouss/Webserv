# ifndef CLIENT_HPP
# define CLIENT_HPP

# include "server_infra.hpp"
# include "request.hpp"
# include <string>


class	client
{

	private:
		int	fd;
		Request	request;
		bool	is_finished_reading;
		size_t	bytes_sent;
		// Buffered response payload for partial writes.
		std::string response_data;
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
		// Access parsed request for routing/response building.
		Request& get_request();
		// Const accessor for request data during response generation.
		const Request& get_request() const;
		// Store serialized response for async send.
		void	set_response(const std::string& data);
		// Read-only response buffer.
		const std::string& get_response() const;
		// Bytes already sent from response buffer.
		size_t	get_bytes_sent() const;
		// Advance the sent offset after a successful send().
		void	add_bytes_sent(size_t n);
		// Reset send offset for a new response.
		void	reset_bytes_sent();
};

# endif