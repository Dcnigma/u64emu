# -------------------------
# Makefile for u64emu on Switch
# -------------------------

# Compiler & tools
CC      := aarch64-none-elf-g++
LD      := aarch64-none-elf-g++
AR      := aarch64-none-elf-ar
OBJCOPY := aarch64-none-elf-objcopy

# Directories
SRC_DIR := src/main
OBJ_DIR := obj
BIN_DIR := release

# Target
TARGET := $(BIN_DIR)/kinx.elf

# Source files
SRCS := $(SRC_DIR)/2100dasm.cpp \
        $(SRC_DIR)/adsp2100.cpp \
        $(SRC_DIR)/iMemory.cpp \
        $(SRC_DIR)/iMemoryOps.cpp \
        $(SRC_DIR)/iBranchOps.cpp \
        $(SRC_DIR)/iCPU.cpp \
        $(SRC_DIR)/iFPOps.cpp \
        $(SRC_DIR)/iATA.cpp \
        $(SRC_DIR)/iMain.cpp \
        $(SRC_DIR)/hleDSP.cpp \
        $(SRC_DIR)/hleMain.cpp \
        $(SRC_DIR)/iRom.cpp \
        $(SRC_DIR)/EmuObject1.cpp \
        $(SRC_DIR)/ki.cpp \
        $(SRC_DIR)/iGeneralOps.cpp \
        $(SRC_DIR)/mmDisplay.cpp \
        $(SRC_DIR)/mmInputDevice.cpp

OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Include paths
INCLUDES := -I"$(SRC_DIR)" \
            -I/opt/devkitpro/libnx/include \
            -I/opt/devkitpro/portlibs/switch/include

# Compiler flags
CFLAGS := -O3 -w -std=c++17 \
          -march=armv8-a -mcpu=cortex-a57+crc+fp+simd \
          -fno-strict-aliasing -fomit-frame-pointer \
          -ffunction-sections -fno-rtti -fno-exceptions \
          $(INCLUDES) -D__SWITCH__

# Linker flags
LDFLAGS := -specs=/opt/devkitpro/libnx/switch.specs \
           -L/opt/devkitpro/libnx/lib \
           -L/opt/devkitpro/portlibs/switch/lib \
           -lglad -lEGL -lglapi -ldrm_nouveau -lnx

# -------------------------
# Rules
# -------------------------

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(LD) $(OBJS) -o $@ $(LDFLAGS)

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# Clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
