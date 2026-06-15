# Windows IME Build

# Cross-compiler settings for MinGW-w64
CC_x64 = x86_64-w64-mingw32-gcc
CC_x86 = i686-w64-mingw32-gcc

# Native Windows compiler (if building on Windows)
CC_NATIVE = gcc

# Flags
CFLAGS = -Wall -O2
LDFLAGS = -shared -limm32 -lole32 -luuid

# Output
TARGET = avro-bangla.dll
TARGET_DEBUG = avro-bangla-debug.dll

# Source files
SRCS = main.c avro_engine.c
OBJS = $(SRCS:.c=.o)

.PHONY: all windows windows64 windows32 clean debug

all: windows

# Build 64-bit Windows DLL (cross-compile from FreeBSD)
windows: windows64

windows64:
	@echo "Building 64-bit Windows DLL..."
	$(CC_x64) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SRCS)
	@echo "Built: $(TARGET)"
	@ls -lh $(TARGET)

windows32:
	@echo "Building 32-bit Windows DLL..."
	$(CC_x86) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SRCS)
	@echo "Built: $(TARGET)"

# Build using native compiler (on Windows with MinGW)
native:
	@echo "Building with native compiler..."
	$(CC_NATIVE) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SRCS)
	@echo "Built: $(TARGET)"

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: windows64
	mv $(TARGET) $(TARGET_DEBUG)
	@echo "Debug build: $(TARGET_DEBUG)"

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TARGET_DEBUG) *.o

# Install helper (copy to Windows system)
install-helper:
	@echo "To install on Windows:"
	@echo "1. Copy $(TARGET) to C:\\Windows\\System32\\"
	@echo "2. Run: regsvr32 C:\\Windows\\System32\\$(TARGET)"
	@echo ""
	@echo "Or use the provided install.reg file"

# Show info
info:
	@echo "Avro Bangla Windows IME Build"
	@echo "=============================="
	@echo "Target: $(TARGET)"
	@echo "Sources: $(SRCS)"
	@echo ""
	@echo "Available targets:"
	@echo "  windows    - Build 64-bit DLL (default)"
	@echo "  windows32  - Build 32-bit DLL"
	@echo "  native     - Build using native compiler"
	@echo "  debug      - Build with debug symbols"
	@echo "  clean      - Remove build artifacts"
	@echo "  info       - Show this information"
