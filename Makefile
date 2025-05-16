NAME = webserv

CXX = c++
DIR := objs/
CXXFLAGS = -Wall -Wextra -Werror -std=c++20

SRCS = main.cpp \
	Server.cpp \
	Config.cpp \
	Client.cpp \
	Request.cpp \
	Response.cpp \
	Utils.cpp

OBJS = $(addprefix $(DIR), $(SRCS:.cpp=.o))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	@echo "\033[1;32m./$(NAME) created!\033[0m"

$(DIR)%.o: %.cpp
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

run: all
	@echo "\033[1;32mRunning ./$(NAME)\033[0m"
	./$(NAME)

.PHONY: all clean fclean re run
