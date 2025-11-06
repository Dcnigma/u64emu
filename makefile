ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)

include $(DEVKITPRO)/libnx/switch_rules

APP_TITLE       := kinx
APP_DESCRIPTION := kinx
APP_AUTHOR      := MVG and DcNigma
APP_VERSION     := 1.0.0
ICON            := logo2.jpg
OUTPUT          := kinx
BINDIR          := release
BUILD           := build

# Tools
LINK  := aarch64-none-elf-g++
CPP   := aarch64-none-elf-g++
RM    := rm -f

# Compiler/Linker flags
DEFINES  := -D__SWITCH__
INCS     := -I"src/main" -I$(DEVKITPRO)/libnx/include -I$(DEVKITPRO)/portlibs/switch/include
CXXFLAGS := $(INCS) $(DEFINES) -march=armv8-a -mcpu=cortex-a57+crc+fp+simd \
            -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections \
            -fno-rtti -fno-exceptions -mtp=soft -fPIE -O3 -w
LIBS     := -specs=$(DEVKITPRO)/libnx/switch.specs -g -march=armv8-a -mtune=cortex-a57 \
            -mtp=soft -fPIE -mcpu=cortex-a57+crc+fp+simd \
            -L$(DEVKITPRO)/libnx/lib -L$(DEVKITPRO)/portlibs/switch/lib \
            -lglad -lEGL -lglapi -ldrm_nouveau -lnx

# Source files
SRC := global.cpp 2100dasm.cpp adsp2100.cpp iMemory.cpp iMemoryOps.cpp \
       iBranchOps.cpp iCPU.cpp iFPOps.cpp iATA.cpp iMain.cpp hleDSP.cpp \
       hleMain.cpp iRom.cpp EmuObject1.cpp ki.cpp iGeneralOps.cpp mmDisplay.cpp mmInputDevice.cpp

OBJ := $(patsubst %.cpp,obj/%.o,$(SRC))

# NACP
ifeq ($(strip $(NO_NACP)),)
NACP := $(BINDIR)/$(OUTPUT).nacp
else
NACP :=
endif

# NRO icon
ifeq ($(strip $(ICON)),)
  ICON_PATH :=
else
  ICON_PATH := --icon=$(TOPDIR)/$(ICON)
endif

.PHONY: all clean

all: $(BINDIR)/$(OUTPUT).nro

# Ensure release folder exists
$(BINDIR):
	mkdir -p $(BINDIR)

# Compile sources
obj/%.o: %.cpp | $(BINDIR)
	$(CPP) -c $< -o $@ $(CXXFLAGS)

# Link ELF
$(BINDIR)/$(OUTPUT).elf: $(OBJ) | $(BINDIR)
	$(LINK) $^ -o $@ $(LIBS)

# Build NRO from ELF
$(BINDIR)/$(OUTPUT).nro: $(BINDIR)/$(OUTPUT).elf $(NACP) | $(BINDIR)
	$(DEVKITPRO)/tools/bin/elf2nro $(BINDIR)/$(OUTPUT).elf $@ $(NACP) $(ICON_PATH)

# Clean
clean:
	$(RM) $(OBJ) $(BINDIR)/$(OUTPUT).elf $(BINDIR)/$(OUTPUT).nro
