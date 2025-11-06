ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

APP_TITLE       := kinx
APP_DESCRIPTION := kinx
APP_AUTHOR      := MVG & DCNigma
APP_VERSION     := 1.0.0
ICON            := logo2.jpg
OUTPUT          := kinx
BINDIR          := release
BUILD           := build

OBJ := obj/global.o obj/2100dasm.o obj/adsp2100.o obj/iMemory.o obj/iMemoryOps.o \
       obj/iBranchOps.o obj/iCPU.o obj/iFPOps.o obj/iATA.o obj/iMain.o obj/hleDSP.o \
       obj/hleMain.o obj/iRom.o obj/EmuObject1.o obj/ki.o obj/iGeneralOps.o obj/mmDisplay.o obj/mmInputDevice.o

LINKOBJ := $(OBJ)
LIBS    := -specs=$(DEVKITPRO)/libnx/switch.specs -g -march=armv8-a -mtune=cortex-a57 \
           -mtp=soft -fPIE -mcpu=cortex-a57+crc+fp+simd \
           -L$(DEVKITPRO)/libnx/lib -L$(DEVKITPRO)/portlibs/switch/lib \
           -lglad -lEGL -lglapi -ldrm_nouveau -lnx

INCS    := -I"src/main" -I$(DEVKITPRO)/libnx/include -I$(DEVKITPRO)/portlibs/switch/include
CXXFLAGS := $(INCS) -D__SWITCH__ -march=armv8-a -mcpu=cortex-a57+crc+fp+simd \
            -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections \
            -fno-rtti -fno-exceptions -mtp=soft -fPIE -O3 -w

LINK   := aarch64-none-elf-g++ 
CPP    := aarch64-none-elf-g++

.PHONY: all clean

all: $(BINDIR)/$(OUTPUT).elf $(BINDIR)/$(OUTPUT).nro

# Compile source files
obj/%.o: %.cpp
	$(CPP) -c $< -o $@ $(CXXFLAGS)

# Link ELF
$(BINDIR)/$(OUTPUT).elf: $(OBJ)
	$(LINK) $(LINKOBJ) -o $@ $(LIBS)

# Build NRO (using libnx switch_rules)
ifeq ($(strip $(NO_NACP)),)
$(BINDIR)/$(OUTPUT).nro: $(BINDIR)/$(OUTPUT).elf $(BINDIR)/$(OUTPUT).nacp
else
$(BINDIR)/$(OUTPUT).nro: $(BINDIR)/$(OUTPUT).elf
endif

clean:
	rm -f $(OBJ) $(BINDIR)/$(OUTPUT).elf $(BINDIR)/$(OUTPUT).nro
