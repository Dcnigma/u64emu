# Switch Homebrew Makefile for u64emu

# Compiler and tools
CC = aarch64-none-elf-g++
CXX = aarch64-none-elf-g++
AR = aarch64-none-elf-ar
RM = rm -f
MKDIR = mkdir -p

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = release

# Switch/LibNX settings
DEVKITPRO ?= /opt/devkitpro
DEVKITA64 ?= $(DEVKITPRO)/devkitA64
LIBNX_DIR ?= $(DEVKITPRO)/libnx
CFLAGS = -D__SWITCH__ -march=armv8-a -mcpu=cortex-a57+crc+fp+simd -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections -fno-rtti -fno-exceptions -mtp=soft -fPIE -O3 -w
LDFLAGS = -specs=$(LIBNX_DIR)/switch.specs -L$(LIBNX_DIR)/lib -L$(DEVKITPRO)/portlibs/switch/lib -lglad -lEGL -lglapi -ldrm_nouveau -lnx

# Source and object files
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))

# Output binary
TARGET = $(BIN_DIR)/kinx.elf

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJECTS)
	$(MKDIR) $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile C++ files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(MKDIR) $(OBJ_DIR)
	$(CXX) -c $< -o $@ $(CFLAGS) -I$(SRC_DIR) -I$(LIBNX_DIR)/include -I$(DEVKITPRO)/portlibs/switch/include

# Clean
clean:
	$(RM) $(OBJ_DIR)/*.o
	$(RM) $(TARGET)

.PHONY: all clean
