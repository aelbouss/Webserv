CPPC = c++
CPPFLAGS =  -Wall -Werror -Wextra
STD = -std=c++98

SRC = $(wildcard infrastructure/*.cpp) $(wildcard request/*.cpp) main.cpp
OBJ = $(SRC:%.cpp=%.o)
TARGET = Main_server

all : $(TARGET)

$(TARGET) : $(OBJ)
	$(CPPC) $(CPPFLAGS) $(STD) $(OBJ) -o $(TARGET)

%.o : %.cpp
	$(CPPC) $(CPPFLAGS) $(STD) -c $< -o $@

clean :
	rm -rf $(OBJ)

fclean : clean
	rm -rf  $(TARGET)

re : fclean all