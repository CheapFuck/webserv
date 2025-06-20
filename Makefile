NAME := webserv
DBNAME := $(NAME)_db

# CXX := c++
CXX := c++# or g++-12
DIR := objs/
DBDIR := db_objs/
CXXFLAGS := -Wall -Wextra -Werror -Wpedantic -Wshadow -std=c++20 -MMD
CXXDBFLAGS := $(CXXFLAGS) -g3 -fsanitize=address,undefined,leak -DDEBUG_MODE -D_GLIBCXX_ASSERTIONS -DFD_TRACKING
MAKEFLAGS += -j $(shell nproc)

SRCS := main.cpp \
	request.cpp \
	client.cpp \
	server.cpp \
	headers.cpp \
	response.cpp \
	methods.cpp \
	requestline.cpp \
	get.cpp \
	timer.cpp \
	post.cpp \
	delete.cpp \
	fd.cpp \
	CGI.cpp \
	sessionManager.cpp \
	cookie.cpp \
	Utils.cpp \
	config/tokenizer.cpp \
	config/lexer.cpp \
	config/consts.cpp \
	config/custom_types/size.cpp \
	config/custom_types/path.cpp \
	config/rules/rules.cpp \
	config/rules/MaxBodySize.cpp \
	config/rules/CGITimeout.cpp \
	config/rules/ServerName.cpp \
	config/rules/Port.cpp \
	config/rules/CGI.cpp \
	config/rules/Method.cpp \
	config/rules/Root.cpp \
	config/rules/Index.cpp \
	config/rules/Redirect.cpp \
	config/rules/ErrorPages.cpp \
	config/rules/AutoIndex.cpp \
	config/rules/UploadDir.cpp \
	config/rules/Location.cpp \
	config/rules/RouteRules.cpp \
	config/rules/ServerConfig.cpp \

OBJS := $(addprefix $(DIR), $(SRCS:.cpp=.o))
DEPS := $(OBJS:%.o=%.d)
DBOBJS := $(addprefix $(DBDIR), $(SRCS:.cpp=.o))

all: $(NAME)
	@$(CXX) --version

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS) -lz
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
	$(CXX) $(CXXDBFLAGS) -o $(DBNAME) $(DBOBJS) -lz
	@echo "\033[1;32m./$(DBNAME) created!\033[0m"
	@$(CXX) --version

$(DBDIR)%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXDBFLAGS) -c $< -o $@

dbrun: $(DBNAME)
	@echo "\033[1;32mRunning ./$(DBNAME)\033[0m"
	./$(DBNAME)

dbrerun: fclean dbrun

gdb: $(DBNAME)
	@echo "\033[1;32mRunning gdb on ./$(DBNAME)\033[0m"
	gdb --args ./$(DBNAME)

-include $(DEPS)

.PHONY: all clean fclean re run rerun debug dbrun dbrerun gdb
