# include "../includes/client.hpp"

client::client() { }

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

void	client::set_client_fd(int clien_fd) { fd = clien_fd ;}


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

