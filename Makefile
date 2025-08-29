# Makefile for MTOG Manager - TI Jacinto 7 (TDA4 J784S4) SoC
# 
# This Makefile compiles the MTOG/TOG manager for OSPAS SW
# Author: OSPAS SW Team
# Date: 2024

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -O2 -g
CFLAGS += -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition
CFLAGS += -Wpointer-arith -Wbad-function-cast -Wnested-externs
CFLAGS += -Wcast-align -Wwrite-strings -Wconversion -Wsign-conversion
CFLAGS += -Wfloat-equal -Wformat=2 -Wundef -Wshadow
CFLAGS += -fstack-protector-strong -D_FORTIFY_SOURCE=2

# For embedded systems, use appropriate cross-compiler
# CC = arm-none-eabi-gcc
# CFLAGS += -mcpu=cortex-a72 -mfpu=neon-fp-armv8 -mfloat-abi=hard
# CFLAGS += -specs=nosys.specs -nostartfiles

# Target executable
TARGET = mtog_manager

# Source files
SOURCES = mtog_manager.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Header files
HEADERS = mtog_manager.h

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Build object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

# Static analysis with cppcheck (if available)
check: $(SOURCES) $(HEADERS)
	@if command -v cppcheck >/dev/null 2>&1; then \
		echo "Running static analysis..."; \
		cppcheck --enable=all --std=c99 --platform=unix64 \
		         --suppress=missingIncludeSystem \
		         --suppress=unusedFunction \
		         --inline-suppr $(SOURCES); \
	else \
		echo "cppcheck not available, skipping static analysis"; \
	fi

# Lint with splint (if available)
lint: $(SOURCES) $(HEADERS)
	@if command -v splint >/dev/null 2>&1; then \
		echo "Running lint analysis..."; \
		splint +posixlib -preproc -weak $(SOURCES); \
	else \
		echo "splint not available, skipping lint analysis"; \
	fi

# Run the test
test: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Clean all generated files
distclean: clean
	rm -f *~ *.bak

# Install (for embedded systems, this would flash to target)
install: $(TARGET)
	@echo "Install target would flash to embedded system"
	@echo "Implementation depends on target board and toolchain"

# Show help
help:
	@echo "Available targets:"
	@echo "  all       - Build the MTOG manager"
	@echo "  check     - Run static analysis (requires cppcheck)"
	@echo "  lint      - Run lint analysis (requires splint)"
	@echo "  test      - Build and run test harness"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo "  install   - Install to target (embedded systems)"
	@echo "  help      - Show this help message"

# Declare phony targets
.PHONY: all check lint test clean distclean install help

# Dependencies
mtog_manager.o: mtog_manager.c mtog_manager.h