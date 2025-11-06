# Makefile for u64emu on Switch

# Compiler & tools
CXX := aarch64-none-elf-g++
CC   := aarch64-none-elf-gcc
RM   := rm -rf

# Directories
SRC_DIR := src/main
OBJ_DIR := obj
BIN_DIR := release

# Target
TARGET := $(BIN_DIR)/kinx.elf

# Flags
CXXFLAGS := -I"$(SRC_DIR)" \
            -I/opt/devkitpro/libnx/include \
            -I/opt/devkitpro/portlibs/switch/include \
            -D__SWITCH__ \
            -march=armv8-a \
            -mcpu=cortex-a57+crc+fp+simd \
            -fno-strict-aliasing \
            -fomit-frame-pointer \
            -ffunction-sections \
            -fno-rtti \
            -fno-exceptions \
            -mtp=soft \
            -fPIE \
            -O3 \
            -w

LDFLAGS := -specs=/opt/devkitpro/libnx/switch.specs \
           -L/opt/devkitpro/libnx/lib \
           -L/opt/devkitpro/portlibs/switch/lib \
           -lglad -lEGL -lglapi -lnx \
           -fPIE

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)

# Object files
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Compile objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# Clean
clean:
	$(RM) $(OBJ_DIR)/*.o $(TARGET)

.PHONY: all clean
