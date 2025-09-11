NAME		= ft_ping
CC			= cc
CFLAGS		= -Wall -Wextra -Werror

INC_DIR		= includes
INC_FLAGS	= -I$(INC_DIR)

SRCS_DIR	= srcs
OBJS_DIR	= objs
SRCS = $(wildcard $(SRCS_DIR)/*.c)
OBJS = $(patsubst $(SRCS_DIR)/%.c,$(OBJS_DIR)/%.o,$(SRCS))

GREEN		= \033[0;32m
CYAN		= \033[0;36m
RESET		= \033[0m
GRAY		= \033[90m

SUCCESS		= $(GREEN)[SUCCESS]$(RESET)
INFO		= $(CYAN)[INFO]$(RESET)

all: $(NAME)

$(OBJS_DIR):
	@if [ ! -d ./$(OBJS_DIR) ]; then \
		echo "$(INFO) Creating $(OBJS_DIR) directory..."; \
		echo "$(GRAY)mkdir -p $(OBJS_DIR)"; \
		mkdir -p $(OBJS_DIR); \
	fi

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c | $(OBJS_DIR)
	mkdir -p $(dir $@)
	@echo "$(INFO) Compiling $<...$(GRAY)"
	$(CC) $(WIN_SIZE) $(CFLAGS) $(INC_FLAGS) -c $< -o $@

$(NAME): $(OBJS)
	@echo "$(INFO) Compiling $(NAME)...$(GRAY)"
	$(CC) $(CFLAGS) $(INC_FLAGS) $(OBJS) -o $@
	@echo "$(SUCCESS) $(NAME) compiled."

clean:
	@echo "$(INFO) Removing object files...$(GRAY)"
	rm -rf $(OBJS_DIR)
	@echo "$(SUCCESS) Objects and libs removed."

fclean: clean
	@echo "$(INFO) Removing $(NAME)...$(GRAY)"
	rm -f $(NAME)
	@echo "$(SUCCESS) $(NAME) removed."

re: fclean all

.PHONY: all clean fclean re
