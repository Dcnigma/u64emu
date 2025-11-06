#--------------------------
# U64Emu Switch Makefile
#--------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#--------------------------
# Output & directories
#--------------------------
APP_TITLE := kinx
APP_AUTHOR := MVG
APP_VERSION := 1.0.0
ICON := logo2.jpg

OBJ       = obj/2100dasm.o obj/adsp2100.o obj/iMemory.o obj/iMemoryOps.o obj/iBranchOps.o obj/iCPU.o \
            obj/iFPOps.o obj/iATA.o obj/iMain.o obj/hleDSP.o obj/hleMain.o obj/iRom.o obj/EmuObject1.o \
            obj/ki.o obj/iGeneralOps.o obj/mmDisplay.o obj/mmInputDevice.o

BIN       = release/kinx.elf
LINK      = aarch64-none-elf-g++
CPP       = aarch64-none-elf-g++
CXXFLAGS  = -I"src/main" -I$(DEVKITPRO)/libnx/include -I$(DEVKITPRO)/portlibs/switch/include \
            -D__SWITCH__ -march=armv8-a -mcpu=cortex-a57+crc+fp+simd \
            -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections \
            -fno-rtti -fno-exceptions -mtp=soft -fPIE -O3 -w
LIBS      = -specs=$(DEVKITPRO)/libnx/switch.specs \
            -L$(DEVKITPRO)/libnx/lib -L$(DEVKITPRO)/portlibs/switch/lib \
            -lglad -lEGL -lglapi -ldrm_nouveau -lnx

#--------------------------
# Targets
#--------------------------
all: $(BIN)

clean:
	rm -rf obj/* release/*

$(BIN): $(OBJ)
	@mkdir -p release
	$(LINK) $(OBJ) -o $(BIN) $(LIBS)

#--------------------------
# Object compilation rules
#--------------------------
obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CPP) -c $< -o $@ $(CXXFLAGS)
