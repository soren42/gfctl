# Makefile for gfctl - Google Fiber Router Control Utility
#
# This Makefile supports building on macOS and Linux.
# Requires libcurl development files.
#
# Build targets:
#   make          - Build the gfctl binary
#   make debug    - Build with debug symbols
#   make clean    - Remove build artifacts
#   make install  - Install to /usr/local/bin
#   make test     - Run basic tests
#

# Project metadata
PROJECT = gfctl
VERSION = 1.0.2

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS += -I$(INCDIR)
LDFLAGS =
LIBS = -lcurl

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS specific flags
    CFLAGS += -D_DARWIN_C_SOURCE
    # Homebrew paths (if curl installed via brew)
    BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
    ifneq ($(BREW_PREFIX),)
        CFLAGS += -I$(BREW_PREFIX)/include
        LDFLAGS += -L$(BREW_PREFIX)/lib
    endif
else ifeq ($(UNAME_S),Linux)
    # Linux specific flags
    CFLAGS += -D_GNU_SOURCE
    LIBS += -lm
endif

# Build type (default: release)
BUILD ?= release

ifeq ($(BUILD),debug)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2 -DNDEBUG
endif

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
DEPENDS = $(OBJECTS:.o=.d)

# Main target
TARGET = $(BINDIR)/$(PROJECT)

# Default target
.PHONY: all
all: $(TARGET)

# Create directories
$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(BINDIR):
	@mkdir -p $(BINDIR)

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS) | $(BINDIR)
	@echo "LD $@"
	@$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@
	@echo "Built $(TARGET) successfully"

# Include dependency files
-include $(DEPENDS)

# Debug build
.PHONY: debug
debug:
	@$(MAKE) BUILD=debug

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning..."
	@rm -rf $(OBJDIR) $(BINDIR)

# Install to system
.PHONY: install
install: $(TARGET)
	@echo "Installing to /usr/local/bin/$(PROJECT)"
	@install -m 755 $(TARGET) /usr/local/bin/$(PROJECT)

# Uninstall from system
.PHONY: uninstall
uninstall:
	@echo "Removing /usr/local/bin/$(PROJECT)"
	@rm -f /usr/local/bin/$(PROJECT)

# Run basic tests
.PHONY: test
test: $(TARGET)
	@echo "Running basic tests..."
	@echo "Test 1: Help output"
	@$(TARGET) --help > /dev/null && echo "  PASS: --help" || echo "  FAIL: --help"
	@echo "Test 2: Version output"
	@$(TARGET) --version > /dev/null && echo "  PASS: --version" || echo "  FAIL: --version"
	@echo "Tests complete"

# Check dependencies
.PHONY: check-deps
check-deps:
	@echo "Checking dependencies..."
	@which curl-config > /dev/null 2>&1 || (echo "ERROR: libcurl not found. Install with:"; \
		echo "  macOS: brew install curl"; \
		echo "  Ubuntu/Debian: apt-get install libcurl4-openssl-dev"; \
		echo "  RHEL/CentOS: yum install libcurl-devel"; \
		exit 1)
	@echo "All dependencies found"

# Print configuration
.PHONY: info
info:
	@echo "Project: $(PROJECT) v$(VERSION)"
	@echo "Platform: $(UNAME_S)"
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "LIBS: $(LIBS)"
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"

# Create distribution tarball
.PHONY: dist
dist: clean
	@echo "Creating distribution tarball..."
	@mkdir -p $(PROJECT)-$(VERSION)
	@cp -r $(SRCDIR) $(INCDIR) Makefile README.md LICENSE $(PROJECT)-$(VERSION)/
	@tar czf $(PROJECT)-$(VERSION).tar.gz $(PROJECT)-$(VERSION)
	@rm -rf $(PROJECT)-$(VERSION)
	@echo "Created $(PROJECT)-$(VERSION).tar.gz"

# Format source code (requires clang-format)
.PHONY: format
format:
	@echo "Formatting source code..."
	@find $(SRCDIR) $(INCDIR) -name '*.c' -o -name '*.h' | xargs clang-format -i

# Static analysis (requires cppcheck)
.PHONY: analyze
analyze:
	@echo "Running static analysis..."
	@cppcheck --enable=all --inconclusive --std=c99 -I$(INCDIR) $(SRCDIR)

# Generate documentation (requires doxygen)
.PHONY: docs
docs:
	@echo "Generating documentation..."
	@doxygen Doxyfile 2>/dev/null || echo "Doxygen not configured"

.PHONY: help
help:
	@echo "gfctl Makefile targets:"
	@echo "  all        - Build the project (default)"
	@echo "  debug      - Build with debug symbols"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to /usr/local/bin"
	@echo "  uninstall  - Remove from /usr/local/bin"
	@echo "  test       - Run basic tests"
	@echo "  check-deps - Verify build dependencies"
	@echo "  info       - Show build configuration"
	@echo "  dist       - Create distribution tarball"
	@echo "  format     - Format source code"
	@echo "  analyze    - Run static analysis"
	@echo "  docs       - Generate documentation"
	@echo "  help       - Show this help"
