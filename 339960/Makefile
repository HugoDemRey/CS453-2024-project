# Makefile

# -----------------------------
# Variables
# -----------------------------

# Build configuration: debug or release
BUILD ?= release

# Directories
INCLUDE_DIR := ../include
SOURCE_DIR  := .
TEST_SRC    := ./tm_test.c  # Adjust this to where tm_test.c is located

# File extensions
EXT_H    := h
EXT_HPP  := h hh hpp hxx h++
EXT_C    := c
EXT_CXX  := C cc cpp cxx c++

# Output binaries
BIN := ../$(notdir $(lastword $(abspath .))).so
TEST_EXEC := tm_test

# Compiler and Linker
CC       := gcc
CXX      := g++
LD       := $(CXX)
AR       := ar
RANLIB   := ranlib

# Compiler Flags
COMMON_CFLAGS   := -Wall -Wextra -Wfatal-errors -std=c11 -fPIC -I$(INCLUDE_DIR)
COMMON_CXXFLAGS := -Wall -Wextra -Wfatal-errors -std=c++17 -fPIC -I$(INCLUDE_DIR)
COMMON_LDFLAGS  := -shared
COMMON_LDLIBS   := -lm -lpthread

# Debug and Release specific flags
ifeq ($(BUILD),debug)
    CFLAGS   := $(COMMON_CFLAGS) -g -O0 -DDEBUG -fsanitize=address
    CXXFLAGS := $(COMMON_CXXFLAGS) -g -O0 -DDEBUG -fsanitize=address
    LDFLAGS  := -fsanitize=address
else
    CFLAGS   := $(COMMON_CFLAGS) -O2
    CXXFLAGS := $(COMMON_CXXFLAGS) -O2
    LDFLAGS  :=
endif

# Source and Header Files
WILD_EXT  = $(strip $(foreach EXT,$($(1)),$(wildcard $(2)/*.$(EXT))))

HDRS_C   := $(call WILD_EXT,EXT_H,$(INCLUDE_DIR))
HDRS_CXX := $(call WILD_EXT,EXT_HPP,$(INCLUDE_DIR))
SRCS_C   := $(call WILD_EXT,EXT_C,$(SOURCE_DIR))
SRCS_CXX := $(call WILD_EXT,EXT_CXX,$(SOURCE_DIR))
OBJS     := $(SRCS_C:.c=.o) $(SRCS_CXX:.cpp=.o)

# Test-related settings
TEST_OBJ := $(TEST_SRC:.c=.o)

# Phony Targets
.PHONY: all build clean run_test debug release

# Default target
all: build

# Build target
build: $(BIN) $(TEST_EXEC)

# Clean target
clean:
	$(RM) $(OBJS) $(BIN) $(TEST_OBJ) $(TEST_EXEC)

# Debug build target
debug: BUILD=debug
debug: build

# Release build target
release: BUILD=release
release: build

# Pattern rules for C files
%.o: %.c $(HDRS_C) Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

# Pattern rules for C++ files
%.o: %.cpp $(HDRS_CXX) Makefile
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Link the shared library
$(BIN): $(OBJS) Makefile
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(COMMON_LDLIBS)

# Compile and link tm_test.c
$(TEST_OBJ): $(TEST_SRC) $(HDRS_C)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_EXEC): $(TEST_OBJ) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(TEST_OBJ) $(OBJS) $(COMMON_LDLIBS)

# Run tests
run_test: $(TEST_EXEC)
	./$(TEST_EXEC)
