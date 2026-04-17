# CC: Defines the compiler. Defaults to gcc but can be overridden 
# for cross-compilation (e.g., CC=aarch64-none-linux-gnu-gcc)
CC ?= $(CROSS_COMPILE)gcc

# CFLAGS: Standard flags (Wall for all warnings, g for debugging)
CFLAGS ?= -Wall -Werror -g

TARGET = aesdsocket
SRC = aesdsocket.c
OBJ = $(SRC:.c=.o)

# Default and all targets (requirement a)
all: $(TARGET)
default: all

# Linking step: creates the executable from object files
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation step: creates object files from source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target: removes build artifacts
clean:
	rm -f $(TARGET) *.o

.PHONY: all default clean