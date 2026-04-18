#pragma once

#include "ServerConfig.hpp"
#include "Location.hpp"
#include "ConfigParser.hpp"
#include "ConfigFile.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <exception>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <cstdint>



template <typename T>
std::string toString(const T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
} 

//Utils
std::string statusCodeString(short statusCode);
unsigned int    fromHexToDec(const std::string &n);