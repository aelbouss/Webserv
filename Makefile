CPPC = c++
CPPFLAGS =  -Wall -Werror -Wextra -g3
STD = -std=c++98

SRC = request/request.cpp \
	responce/response.cpp \
	CGI_handling/CgiHandler.cpp \
	CGI_handling/ConfigParser.cpp \
	CGI_handling/ConfigFile.cpp \
	CGI_handling/ServerConfig.cpp \
	CGI_handling/Location.cpp \
	CGI_handling/Utils.cpp \
	infrastructure/client.cpp \
	infrastructure/multi_listener_setup.cpp \
	infrastructure/multiplexing.cpp \
	infrastructure/SessionManager.cpp \
	infrastructure/ServerManager.cpp \
	infrastructure/server_infra.cpp  \
	main.cpp

HEADERS = $(wildcard includes/*.hpp)

OBJ = $(SRC:%.cpp=%.o)
NAME = webserv

all : $(NAME)

$(NAME) : $(OBJ)
	$(CPPC) $(CPPFLAGS) $(STD) $(OBJ) -o $(NAME)

%.o : %.cpp $(HEADERS)
	$(CPPC) $(CPPFLAGS) $(STD) -c $< -o $@

clean :
	rm -rf $(OBJ)

fclean : clean
	rm -rf  $(NAME)

re : fclean all