# Simple recursive Makefile for u64emu Switch build

# Compiler and tools
CXX = aarch64-none-elf-g++
RM = rm -rf
MKDIR = mkdir -p

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = release

# Switch/LibNX settings
DEVKITPRO ?= /opt/devkitpro
DEVKITA64 ?= $(DEVKITPRO)/devkitA64
LIBNX_DIR ?= $(DEVKITPRO)/libnx
CXXFLAGS = -D__SWITCH__ -march=armv8-a -mcpu=cortex-a57+crc+fp+simd \
           -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections \
           -fno-rtti -fno-exceptions -mtp=soft -fPIE -O3 -w \
           -I$(SRC_DIR) -I$(LIBNX_DIR)/include -I$(DEVKITPRO)/portlibs/switch/include
LDFLAGS = -specs=$(LIBNX_DIR)/switch.specs \
          -L$(LIBNX_DIR)/lib -L$(DEVKITPRO)/portlibs/switch/lib \
          -lglad -lEGL -lglapi -ldrm_nouveau -lnx

# Recursive search for source files
SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')
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
	$(MKDIR) $(dir $@)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# Clean
clean:
	$(RM) $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
