# ifndef MULTI_LISTENER_SETUP_HPP
# define MULTI_LISTENER_SETUP_HPP

// create a data structure that holds  the interface and port pair and  the max size body for every server block

# include <iostream>
 
# include <unistd.h>

/*t - he clas that will store the infos retrieved from config file - */

class	engine_resource
{
	private:
		std::string	interface;
		int	port;
		size_t	max_size_body;
	public:
		engine_resource();
		engine_resource(std::string inter, int port , size_t max_body_s);
		~engine_resource();
		std::string	get_interface();
		int	get_port();
		size_t	get_size_body();
};

# endif