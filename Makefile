CPPC = c++
CPPFLAGS =  -Wall -Werror -Wextra
STD = -std=c++98

SRC = $(wildcard infrastructure/*.cpp) $(wildcard request/*.cpp) \
	responce/response.cpp \
	CGI_handling/CgiHandler.cpp \
	CGI_handling/ConfigParser.cpp \
	CGI_handling/ConfigFile.cpp \
	CGI_handling/ServerConfig.cpp \
	CGI_handling/Location.cpp \
	CGI_handling/Utils.cpp \
	main.cpp
HEADERS = $(wildcard includes/*.hpp)

OBJ = $(SRC:%.cpp=%.o)
TARGET = Main_server

all : $(TARGET)

$(TARGET) : $(OBJ)
	$(CPPC) $(CPPFLAGS) $(STD) $(OBJ) -o $(TARGET)

%.o : %.cpp $(HEADERS)
	$(CPPC) $(CPPFLAGS) $(STD) -c $< -o $@

clean :
	rm -rf $(OBJ)

fclean : clean
	rm -rf  $(TARGET)

re : fclean all