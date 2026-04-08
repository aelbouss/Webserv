# include "../inc/multi_listener_setup.hpp"


engine_resource::engine_resource(std::string inter, int Port , size_t max_body_s) :interface(inter) , port(Port) , max_size_body(max_body_s) {}

std::string		engine_resource::get_interface() { return (interface); }

int	engine_resource::get_port() { return (port); }

size_t	engine_resource::get_size_body() { return (max_size_body); }

engine_resource::~engine_resource() {}