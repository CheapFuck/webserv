NAME := webserv
DBNAME := $(NAME)_db

# CXX := c++
CXX := c++# or g++-12
DIR := objs/
DBDIR := db_objs/
CXXFLAGS := -Wall -Wextra -Werror -Wpedantic -std=c++20 -MMD -I includes/
CXXDBFLAGS := $(CXXFLAGS) -g3 -fsanitize=address,undefined,leak -DDEBUG_MODE -D_GLIBCXX_ASSERTIONS -DFD_TRACKING
MAKEFLAGS += -j $(shell nproc)

SRCS := src/main.cpp \
	src/request.cpp \
	src/client.cpp \
	src/server.cpp \
	src/headers.cpp \
	src/response.cpp \
	src/methods.cpp \
	src/requestline.cpp \
	src/timer.cpp \
	src/fd.cpp \
	src/CGI.cpp \
	src/fdReader.cpp \
	src/sessionManager.cpp \
	src/cookie.cpp \
	src/Utils.cpp \
	src/config/arena.cpp \
	src/config/config.cpp \
	src/config/lexer.cpp \
	src/config/parser.cpp \
	src/config/parserExceptions.cpp \
	src/config/types/consts.cpp \
	src/config/types/path.cpp \
	src/config/types/size.cpp \
	src/config/types/timespan.cpp \
	src/config/rules/objectParser.cpp \
	src/config/rules/ruleParser.cpp \
	src/config/rules/ruleTemplates/aliasRule.cpp \
	src/config/rules/ruleTemplates/autoindexRule.cpp \
	src/config/rules/ruleTemplates/bodyReadTimeoutRule.cpp \
	src/config/rules/ruleTemplates/headerReadTimeoutRule.cpp \
	src/config/rules/ruleTemplates/cgiExtensionRule.cpp \
	src/config/rules/ruleTemplates/cgiRule.cpp \
	src/config/rules/ruleTemplates/httpRule.cpp \
	src/config/rules/ruleTemplates/keepaliveReadTimeoutRule.cpp \
	src/config/rules/ruleTemplates/cgiTimeoutRule.cpp \
	src/config/rules/ruleTemplates/defineRule.cpp \
	src/config/rules/ruleTemplates/errorpageRule.cpp \
	src/config/rules/ruleTemplates/includeRule.cpp \
	src/config/rules/ruleTemplates/indexRule.cpp \
	src/config/rules/ruleTemplates/locationRule.cpp \
	src/config/rules/ruleTemplates/maxBodySizeRule.cpp \
	src/config/rules/ruleTemplates/methodsRule.cpp \
	src/config/rules/ruleTemplates/portRule.cpp \
	src/config/rules/ruleTemplates/returnRule.cpp \
	src/config/rules/ruleTemplates/rootRule.cpp \
	src/config/rules/ruleTemplates/serverconfigRule.cpp \
	src/config/rules/ruleTemplates/servernameRule.cpp \
	src/config/rules/ruleTemplates/uploadstoreRule.cpp

OBJS := $(addprefix $(DIR), $(SRCS:.cpp=.o))
DEPS := $(OBJS:%.o=%.d)
DBOBJS := $(addprefix $(DBDIR), $(SRCS:.cpp=.o))
DBDEPS := $(DBOBJS:%.o=%.d)

all: $(NAME)
	echo $(SRCS)

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

testfile:
	fallocate -l 1G testfile

analyze:
	cppcheck --enable=all --suppress=missingInclude --suppress=missingIncludeSystem --std=c++20 $(SRCS) 
# ./cppcheck --enable=all --suppress=missingInclude --suppress=missingIncludeSystem --std=c++20 ../src

gdb: $(DBNAME)
	@echo "\033[1;32mRunning gdb on ./$(DBNAME)\033[0m"
	gdb --args ./$(DBNAME)

-include $(DEPS)
-include $(DBDEPS)

.PHONY: all clean fclean re run rerun debug dbrun dbrerun gdb
