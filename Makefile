# Compiler and Flags
CXX      := clang++
CXXFLAGS := -std=c++23 -Wall -Wextra -O2 -Iinclude -MMD -MP

# Directories
SRC_DIR  := src
OBJ_DIR  := build/obj
BIN_DIR  := build/bin

# Target executable name
TARGET   := $(BIN_DIR)/opengb

# Automatically find all .cpp files in src/
SRCS     := $(wildcard $(SRC_DIR)/*.cpp)

# Map each .cpp file to a corresponding .o file inside build/obj/
OBJS     := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Include automatically generated dependency files (.d)
DEPS     := $(OBJS:.o=.d)

# Default target when running 'make'
all: $(TARGET)

# Rule to link the final executable from object files
$(TARGET): $(OBJS) | $(BIN_DIR)
	rm -f $@
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@
	@echo "Build successful: $@"

# Rule to compile each .cpp file into a .o file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create output directories if they don't exist
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

# Include .d files (ignores errors if they don't exist yet)
-include $(DEPS)

# Clean built files
clean:
	rm -rf build
	@echo "Cleaned build directory."

# Phony targets (targets that aren't actual files)
.PHONY: all clean run
