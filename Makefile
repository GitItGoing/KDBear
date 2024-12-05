# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Iinclude
LDFLAGS = include/e.o

# Directories
SRC_DIR = src
BUILD_DIR = build
TEST_DIR = unit_tests
DEMO_DIR = demo

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
TEST_SRCS = $(wildcard $(TEST_DIR)/*.cpp)
TEST_TARGETS = $(patsubst $(TEST_DIR)/%.cpp, $(BUILD_DIR)/%, $(TEST_SRCS))
DEMO_SRCS = $(wildcard $(DEMO_DIR)/main.cpp)

# Executable name
TARGET = kdbear_demo

# Default target
all: $(TARGET)

# Main program target
$(TARGET): $(OBJS) $(BUILD_DIR)/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Object files for source
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Object file for main demo
$(BUILD_DIR)/main.o: $(DEMO_DIR)/main.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Test targets
test: $(TEST_TARGETS)
	@for test in $(TEST_TARGETS); do \
		echo "Running $$test"; \
		./$$test || exit 1; \
	done

$(BUILD_DIR)/%: $(TEST_DIR)/%.cpp $(OBJS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Clean up build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET) run_tests
