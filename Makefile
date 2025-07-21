NAME := webserv
DBNAME := $(NAME)_db

# CXX := c++
CXX := c++# or g++-12
DIR := objs/
DBDIR := db_objs/
CXXFLAGS := -Wall -Wextra -Werror -Wpedantic -std=c++20 -MMD
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
	timer.cpp \
	fd.cpp \
	CGI.cpp \
	fdReader.cpp \
	sessionManager.cpp \
	cookie.cpp \
	Utils.cpp \
	config/arena.cpp \
	config/config.cpp \
	config/lexer.cpp \
	config/parser.cpp \
	config/parserExceptions.cpp \
	config/types/consts.cpp \
	config/types/path.cpp \
	config/types/size.cpp \
	config/types/timespan.cpp \
	config/rules/objectParser.cpp \
	config/rules/ruleParser.cpp \
	config/rules/ruleTemplates/aliasRule.cpp \
	config/rules/ruleTemplates/autoindexRule.cpp \
	config/rules/ruleTemplates/cgiExtensionRule.cpp \
	config/rules/ruleTemplates/cgiRule.cpp \
	config/rules/ruleTemplates/cgiTimeoutRule.cpp \
	config/rules/ruleTemplates/defineRule.cpp \
	config/rules/ruleTemplates/errorpageRule.cpp \
	config/rules/ruleTemplates/includeRule.cpp \
	config/rules/ruleTemplates/indexRule.cpp \
	config/rules/ruleTemplates/locationRule.cpp \
	config/rules/ruleTemplates/maxBodySizeRule.cpp \
	config/rules/ruleTemplates/methodsRule.cpp \
	config/rules/ruleTemplates/portRule.cpp \
	config/rules/ruleTemplates/returnRule.cpp \
	config/rules/ruleTemplates/rootRule.cpp \
	config/rules/ruleTemplates/serverconfigRule.cpp \
	config/rules/ruleTemplates/servernameRule.cpp \
	config/rules/ruleTemplates/uploadstoreRule.cpp

OBJS := $(addprefix $(DIR), $(SRCS:.cpp=.o))
DEPS := $(OBJS:%.o=%.d)
DBOBJS := $(addprefix $(DBDIR), $(SRCS:.cpp=.o))
DBDEPS := $(DBOBJS:%.o=%.d)

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
-include $(DBDEPS)

.PHONY: all clean fclean re run rerun debug dbrun dbrerun gdb
