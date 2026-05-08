# include "../includes/client.hpp"

// Initialize request/response state for a new client.
client::client() : is_finished_reading(false), bytes_sent(0) { }

client&	client::operator = (const client& src)
{
	if (this == &src)
		return (*this);

	this->fd = src.fd;
	this->request = src.request;
	this->bytes_sent = src.bytes_sent;
	this->is_finished_reading = src.is_finished_reading;

	return (*this);
}


client::client(const client& src)
{
	(*this) = src ;
}

client::~client() {}

// Assign fd and reset per-connection response state.
void	client::set_client_fd(int clien_fd)
{
	fd = clien_fd;
	bytes_sent = 0;
	response_data.clear();
}


void	client::set_finished_reading(bool var) {  is_finished_reading = var; }

bool	client::get_finished_reading() { return (is_finished_reading); }

int	client::get_client_id(){ return(fd); }


void	client::parse_request(char *data, int rb)
{
	request.parse(data, rb);
}

bool	client::is_parsing_finished()
{
	return (request.isFinished()) ;
}

// Mutable request accessor for parser integration.
Request& client::get_request()
{
	return request;
}

// Const request accessor for routing/response generation.
const Request& client::get_request() const
{
	return request;
}

// Store serialized response for later partial sends.
void client::set_response(const std::string& data)
{
	response_data = data;
}

// Access response payload for send().
const std::string& client::get_response() const
{
	return response_data;
}

// Track how many bytes were already written to the socket.
size_t client::get_bytes_sent() const
{
	return bytes_sent;
}

// Advance the send cursor after a successful write.
void client::add_bytes_sent(size_t n)
{
	bytes_sent += n;
}

// Reset send cursor when preparing a new response.
void client::reset_bytes_sent()
{
	bytes_sent = 0;
}

