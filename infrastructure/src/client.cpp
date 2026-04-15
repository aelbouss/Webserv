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

size_t	client::get_request_size()
{
	return(request_buffer.size());
}

int client::check_headers_is_finish()
{
	size_t	idx;

	idx = request_buffer.find("\r\n\r\n");
	if (idx != std::string::npos)	
		return (1);
	return (-1);
}

void	client::set_content_length(size_t nb)
{
	content_length = nb;
}
void	client::set_header_size(size_t nb)
{
	header_size = nb;
}
void	client::set_headers_complete(bool var)
{
	content_length = var;
}

bool	client::get_headers_complete()
{
	return (content_length);
}

size_t	client::get_content_length()
{
	return (content_length);
}
size_t	client::get_header_size()
{
	return (header_size);
}

