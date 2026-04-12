# include "../inc/client.hpp"

client::client() { }

client&	client::operator = (const client& src)
{
	if (this == &src)
		return (*this);
	this->client_id = src.client_id ;
	this->request_buffer = src.request_buffer;
	return (*this);
}


client::client(const client& src)
{
	(*this) = src ;
}

client::~client() {}


int	client::get_client_id(){ return (client_id); }


std::string client::get_request() { return (request_buffer) ; }

void	client::set_id(int fd)
{
	client_id = fd;
}

void	client::append_request(char *raw_bytes)
{
	if (raw_bytes == NULL)
	{
		std::cerr << "NULL reached" << std::endl;
		return ;
	}
	std::string str(raw_bytes);
	request_buffer.append(raw_bytes);
}