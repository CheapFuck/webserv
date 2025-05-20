NAME := webserv
DBNAME := $(NAME)_db

CXX := c++
DIR := objs/
DBDIR := db_objs/
CXXFLAGS := -Wall -Wextra -Werror -std=c++20
CXXDBFLAGS := -g -fsanitize=address

SRCS := main.cpp \
	client.cpp \
	server.cpp \
	config/tokenizer.cpp \
	config/lexer.cpp \
	config/consts.cpp \
	config/custom_types/size.cpp \
	config/custom_types/path.cpp \
	config/rules/MaxBodySize.cpp \
	config/rules/ServerName.cpp \
	config/rules/Port.cpp \
	config/rules/Method.cpp \
	config/rules/Root.cpp \
	config/rules/Index.cpp \
	config/rules/Redirect.cpp \
	config/rules/ErrorPages.cpp \
	config/rules/AutoIndex.cpp \
	config/rules/UploadDir.cpp \
	config/rules/Location.cpp \
	config/rules/RouteRules.cpp \
	config/rules/ServerConfig.cpp

OBJS := $(addprefix $(DIR), $(SRCS:.cpp=.o))
DBOBJS := $(addprefix $(DBDIR), $(SRCS:.cpp=.o))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	@echo "\033[1;32m./$(NAME) created!\033[0m"

$(DIR)%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(DIR)
	rm -rf $(DBDIR)

fclean: clean
	rm -f $(NAME)
	rm -f $(DBNAME)

re: fclean all

run: all
	@echo "\033[1;32mRunning ./$(NAME)\033[0m"
	./$(NAME)

rerun: fclean run

debug: $(DBNAME)

$(DBNAME): $(DBOBJS)
	$(CXX) $(CXXDBFLAGS) -o $(DBNAME) $(DBOBJS)
	@echo "\033[1;32m./$(DBNAME) created!\033[0m"


$(DBDIR)%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXDBFLAGS) -c $< -o $@

dbrun: $(DBNAME)
	@echo "\033[1;32mRunning ./$(DBNAME)\033[0m"
	./$(DBNAME)

gdb: $(DBNAME)
	@echo "\033[1;32mRunning gdb on ./$(DBNAME)\033[0m"
	gdb --args ./$(DBNAME)

.PHONY: all clean fclean re run rerun debug dbrun gdb