# --------------------------------------------------
# u64emu Makefile for Nintendo Switch (libnx)
# --------------------------------------------------

# Compiler / Linker
CC := aarch64-none-elf-g++
CXX := aarch64-none-elf-g++
LD := aarch64-none-elf-g++

# Directories
SRC_DIR := src
OBJ_DIR := obj
BUILD_DIR := release

# Flags
CXXFLAGS := -I$(SRC_DIR) -I/opt/devkitpro/libnx/include -I/opt/devkitpro/portlibs/switch/include \
    -D__SWITCH__ -march=armv8-a -mcpu=cortex-a57+crc+fp+simd \
    -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections \
    -fno-rtti -fno-exceptions -mtp=soft -fPIE -O3 -w

LDFLAGS := -specs=/opt/devkitpro/libnx/switch.specs \
    -L/opt/devkitpro/libnx/lib -L/opt/devkitpro/portlibs/switch/lib \
    -lglad -lEGL -lglapi -ldrm_nouveau -lnx

# Sources & Objects
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Target
TARGET := $(BUILD_DIR)/kinx.elf

# --------------------------------------------------
# Rules
# --------------------------------------------------

all: $(OBJ_DIR) $(BUILD_DIR) $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile C++ files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# Link
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)
