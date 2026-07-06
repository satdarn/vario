# Compiler and compilation flags
CC      = gcc
CFLAGS  = -Wall -Wextra -g -O0 -Iinclude -I.
LDFLAGS = 

# Output binary name
TARGET  = bin/vlc

# Find all .c files recursively in src/
SRCS    = $(shell find src -name "*.c" -type f)

# Generate object file paths in obj/ mirroring src/ structure
OBJS    = $(patsubst src/%.c, obj/%.o, $(SRCS))

# Generate dependency files
DEPS    = $(OBJS:.o=.d)

# Default target
all: $(TARGET)

# Debug build with address sanitizer
debug: CFLAGS += -fsanitize=address
debug: LDFLAGS += -fsanitize=address
debug: $(TARGET)

# Link
$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)

# Compile with dependency generation
obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Run
run: $(TARGET)
	./$(TARGET) $(input)

# Clean
clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

# Distclean
distclean:
	rm -rf obj/ bin/

test: $(TARGET)
	./$(TARGET) test.var var 

context: $(TARGET) 
	context .
	echo "./$(TARGET) test.var var" >> context_dump.md
	./$(TARGET) test.var var &>> context_dump.md

# Help
help:
	@echo "Available targets:"
	@echo "  all         - Build the vlc binary (default)"
	@echo "  debug       - Build with address sanitizer"
	@echo "  clean       - Remove object files, deps, and binary"
	@echo "  distclean   - Remove obj/ and bin/ directories entirely"
	@echo "  run         - Run the interpreter (usage: make run input=filename)"
	@echo "  help        - Show this help message"

# Include dependency files
-include $(DEPS)

.PHONY: all debug clean distclean run help
