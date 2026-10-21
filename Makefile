# Detect the operating system
UNAME_S := $(shell uname -s)

# Choose the default compiler based on the operating system
ifeq ($(UNAME_S),Darwin)
    # macOS use clang++ by default
    CXX ?= clang++
else
    # Linux / WSL use g++ by default
    CXX ?= g++
endif

# Compiler and Flags
# Apple clang does not know -std=c++23, use -std=c++2b instead
ifeq ($(UNAME_S),Darwin)
    CXX_STD ?= -std=c++2b
else
    CXX_STD ?= -std=c++23
endif

CXXFLAGS ?= -Wall -Wextra $(CXX_STD) -O3 -Iinclude
LDFLAGS  ?=

# SDL2 (Homebrew / system install via pkg-config)
# Can be overridden on the command line: make SDL2_CFLAGS=... SDL2_LIBS=...
SDL2_CFLAGS ?= $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL2_LIBS   ?= $(shell pkg-config --libs sdl2 2>/dev/null)
ifeq ($(strip $(SDL2_LIBS)),)
    # Fallback when pkg-config is unavailable or finds nothing
    ifeq ($(UNAME_S),Darwin)
        BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
        ifeq ($(strip $(BREW_PREFIX)),)
            BREW_PREFIX := /opt/homebrew
        endif
        SDL2_CFLAGS := -I$(BREW_PREFIX)/include/SDL2 -I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks
        SDL2_LIBS   := -L$(BREW_PREFIX)/lib -lSDL2 -lSDL2_ttf
    endif
endif
CXXFLAGS    += $(SDL2_CFLAGS)
LDFLAGS     += $(SDL2_LIBS)

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
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $@
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
