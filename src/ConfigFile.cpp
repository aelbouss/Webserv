#include "../inc/ConfigFile.hpp"



ConfigFile::ConfigFile() : _size(0) { }

ConfigFile::ConfigFile(std::string const path) : _path(path), _size(0) { }

ConfigFile::~ConfigFile() { }


/* Classify a path as regular file (1), directory (2), other node (3), or missing (-1). */
int ConfigFile::getTypePath(std::string const path)
{
	struct stat	buffer;
	int			result;
	
	result = stat(path.c_str(), &buffer);
	if (result == 0)
	{
		if (buffer.st_mode & S_IFREG)
			return (1);
		else if (buffer.st_mode & S_IFDIR)
			return (2);
		else
			return (3);
	}
	else
		return (-1);
}

/* Wrapper around access(2): returns 0 when path satisfies the requested mode. */
int	ConfigFile::checkFile(std::string const path, int mode)
{
	return (access(path.c_str(), mode));
}

int ConfigFile::isFileExistAndReadable(std::string const path, std::string const index)
{
	if (getTypePath(index) == 1 && checkFile(index, R_OK) == 0)
		return (0);
	if (getTypePath(path + index) == 1 && checkFile(path + index, R_OK) == 0)
		return (0);
	return (-1);
}

/* Read full file contents into a single string buffer. */
std::string	ConfigFile::readFile(std::string path)
{
	if (path.empty() || path.length() == 0)
		return ("");
	std::ifstream config_file(path.c_str());
	if (!config_file || !config_file.is_open())
		return ("");

	std::stringstream buffer;
	buffer << config_file.rdbuf();
	return (buffer.str());
}

/* Get functions */
std::string ConfigFile::getPath()
{
	return (this->_path);
}

int ConfigFile::getSize()
{
	return (this->_size);
}