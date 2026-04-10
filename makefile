# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Target executable
TARGET = solution

# Source files
SOURCES = solution.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Default target - builds the executable
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET) game_output.csv game_input.csv

# Run pytest tests
test: $(TARGET)
	pytest test_game.py -v

# Run the game
run: $(TARGET)
	./$(TARGET)

# Phony targets (not actual files)
.PHONY: all clean test run