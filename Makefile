CC = gcc
CFLAGS = -Wall -Wextra
# CFLAGS = -Wall -Wextra -Werror

TARGET = csvreader

SRCS = main.c libcsvparse.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

# Link object files into final binary
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)

# Force rebuild
re: clean all

.PHONY: all clean re
