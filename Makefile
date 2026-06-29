# Compiler and compilation flags
CC      = gcc
CFLAGS  = -Wall -Wextra -g -O0 -Iinterpreter  # -fsanitize=address 

# Output binary name
TARGET  = bin/vlc

# Source files (add new .c files here if you create them)
SRCS    = interpreter/main.c \
          interpreter/common.c \
          interpreter/parser.c \
          interpreter/nodes.c \
          interpreter/util.c \
          interpreter/sema.c

# Map source files to object files in the obj/ folder
OBJS    = $(patsubst interpreter/%.c, obj/%.o, $(SRCS))

# Default target: builds the entire interpreter executable
all: $(TARGET)


debug: $(OBJS)
	@mkdir -p bin
	$(CC) $(CFLAGS) -fsanitize=address $(OBJS) -o $(TARGET)

# Rule to link the object files into the final executable binary
$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Pattern rule to compile each individual .c source file into a .o object file
obj/%.o: interpreter/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

# Run the interpreter with a variable file argument (e.g., make run input=new.var)
run: $(TARGET)
	./$(TARGET) $(input)

# Clean up build artifacts (leaves directories intact)
clean:
	rm -f obj/*.o $(TARGET)

# Completely wipe out built artifacts and clean directories entirely
distclean:
	rm -rf obj/ bin/

# Show helpful targets
help:
	@echo "Available targets:"
	@echo "  all         - Build the vlc binary (default)"
	@echo "  clean       - Remove object files and binary"
	@echo "  distclean   - Remove obj/ and bin/ directories entirely"
	@echo "  run         - Run the interpreter (usage: make run input=filename)"
	@echo "  help        - Show this help message"

.PHONY: all clean distclean run help
