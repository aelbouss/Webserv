#pragma once
//#include "Webserv.hpp"
#include "CgiHandler.hpp"

class ConfigFile {
	private:
		std::string		_path;
		size_t			_size;

	public:
		ConfigFile();
		ConfigFile(std::string const path);
		~ConfigFile();

		static int getTypePath(std::string const path);
		static int checkFile(std::string const path, int mode);
		std::string	readFile(std::string path);
		static int isFileExistAndReadable(std::string const path, std::string const index);

		std::string getPath();
		int getSize();
};
