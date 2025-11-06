# ----------------------
# U64EMU Makefile for Switch (LibNX)
# ----------------------

# Compiler and tools
CC      := aarch64-none-elf-g++
CXX     := aarch64-none-elf-g++
LD      := aarch64-none-elf-g++
AR      := aarch64-none-elf-ar
RM      := rm -f

# Project directories
SRC_DIR := src/main
OBJ_DIR := obj
BIN_DIR := release

# Switch/LibNX includes and libs
INC     := -I$(SRC_DIR) \
           -I/opt/devkitpro/libnx/include \
           -I/opt/devkitpro/portlibs/switch/include
LIBS    := -L/opt/devkitpro/libnx/lib \
           -L/opt/devkitpro/portlibs/switch/lib \
           -lglad -lEGL -lglapi -ldrm_nouveau -lnx

# Compiler flags
CFLAGS  := -D__SWITCH__ -march=armv8-a -mcpu=cortex-a57+crc+fp+simd \
           -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections \
           -fno-rtti -fno-exceptions -mtp=soft -fPIE -O3 -w

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Output binary
TARGET := $(BIN_DIR)/kinx.elf

# Default target
all: $(TARGET)

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CFLAGS) $(INC) -c $< -o $@

# Link
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(LD) $(OBJS) -o $(TARGET) -specs=/opt/devkitpro/libnx/switch.specs $(CFLAGS) $(LIBS)

# Directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean
clean:
	$(RM) $(OBJ_DIR)/*.o
	$(RM) $(TARGET)
