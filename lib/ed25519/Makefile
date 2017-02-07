# Declaration of variables
CC = g++
CC_FLAGS = -w -D__STDC_CONSTANT_MACROS

# File names
EXEC = libed25519.a
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

# Main target
$(EXEC): $(OBJECTS)
	ar rvs $(EXEC) $(OBJECTS)

# To obtain object files
%.o: %.cpp
	$(CC) -c $(CC_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS)
