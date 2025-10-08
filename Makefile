NAME		:= webserv

include sources.mk

BUILD_DIR	:= .build/
OBJS 		:= $(patsubst %.cpp,$(BUILD_DIR)%.o,$(SRCS))
DEPS		:= $(OBJS:.o=.d)

# ********** FLAGS - COMPILATION FLAGS - OPTIONS ***************************** #

CXX			:= c++
CFLAGS		:= -Wall -Wextra -Werror -std=c++98 -g3
CPPFLAGS	:= -MMD -MP -I incs/

RM			:= rm -f
RMDIR		:= -r
MAKEFLAGS	+= --no-print-directory
DIR_DUP		:= mkdir -p $(BUILD_DIR)

.DEFAULT_GOAL	:= all

# ********** RULES *********************************************************** #

.PHONY: all
all: $(NAME)

$(NAME): Makefile $(OBJS)
	@$(CXX) $(CFLAGS) $(CPPFLAGS) -o $(NAME) $(OBJS)
	@echo "\n$(GREEN_BOLD)✓ $(NAME) is ready$(RESETC)"

$(BUILD_DIR)%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "$(CYAN)[Compiling]$(RESETC) $<"
	@$(CXX) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@$(RM) $(OBJS) $(DEPS)
	@echo "$(RED_BOLD)[Cleaning]$(RESETC)"

.PHONY: fclean
fclean: clean
	@$(RM) $(RMDIR) $(NAME) $(BUILD_DIR)
	@echo "$(RED_BOLD)✓ $(NAME) is fully cleaned!$(RESETC)"

.PHONY: re
re: fclean all

# ********** COLORS AND BACKGROUND COLORS ************************************ #

RESETC				:= \033[0m

GREEN_BOLD			:= \033[1;32m
RED_BOLD			:= \033[1;31m
CYAN				:= \033[0;36m

-include $(DEPS)
