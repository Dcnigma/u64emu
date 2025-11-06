ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

export BUILD_EXEFS_SRC := build/exefs

APP_TITLE       := kinx
APP_DESCRIPTION := kinx
APP_AUTHOR      := MVG & DCNigma
APP_VERSION     := 1.0.0
ICON            := logo2.jpg

# Compiler and linker
CPP    := aarch64-none-elf-g++
LINK   := aarch64-none-elf-g++
RM     := rm -f

# Output directories and files
BUILD  := build
BINDIR := release
OUTPUT := kinx
BIN    := $(BINDIR)/$(OUTPUT).elf

# Sources and objects
SRC    := global.cpp 2100dasm.cpp adsp2100.cpp iMemory.cpp iMemoryOps.cpp iBranchOps.cpp \
          iCPU.cpp iFPOps.cpp iATA.cpp iMain.cpp hleDSP.cpp hleMain.cpp iRom.cpp \
          EmuObject1.cpp ki.cpp iGeneralOps.cpp mmDisplay.cpp mmInputDevice.cpp
OBJ    := $(patsubst %.cpp,obj/%.o,$(SRC))
LINKOBJ := $(OBJ)

# Includes and defines
INCS    := -I"src/main" -I$(DEVKITPRO)/libnx/include -I$(DEVKITPRO)/portlibs/switch/include
DEFINES := -D__SWITCH__
CXXFLAGS := $(INCS) $(DEFINES) -march=armv8-a -mcpu=cortex-a57+crc+fp+simd \
            -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections \
            -fno-rtti -fno-exceptions -mtp=soft -fPIE -O3 -w
LIBS    := -specs=$(DEVKITPRO)/libnx/switch.specs -g -march=armv8-a -mtune=cortex-a57 \
            -mtp=soft -fPIE -mcpu=cortex-a57+crc+fp+simd \
            -L$(DEVKITPRO)/libnx/lib -L$(DEVKITPRO)/portlibs/switch/lib \
            -lglad -lEGL -lglapi -ldrm_nouveau -lnx

# NRO flags
NROFLAGS := --icon=$(TOPDIR)/$(ICON) --nacp=$(BINDIR)/$(OUTPUT).nacp

#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
.PHONY: all clean

all: $(BINDIR)/$(OUTPUT).nro

clean:
	$(RM) $(OBJ) $(BIN) $(BINDIR)/$(OUTPUT).nro $(BINDIR)/$(OUTPUT).nacp \
	        $(BINDIR)/$(OUTPUT).pfs0 $(BINDIR)/$(OUTPUT).nso

# Compile object files
obj/%.o: %.cpp
	@mkdir -p obj
	$(CPP) -c $< -o $@ $(CXXFLAGS)

# Link ELF
$(BIN): $(OBJ)
	@mkdir -p $(BINDIR)
	$(LINK) $(LINKOBJ) -o $@ $(LIBS)

# Generate NACP
$(BINDIR)/$(OUTPUT).nacp:
	@mkdir -p $(BINDIR)
	nx_generate_nacp $@ \
		--title "$(APP_TITLE)" \
		--author "$(APP_AUTHOR)" \
		--version "$(APP_VERSION)" \
		--icon "$(ICON)"

# Generate NSO (optional, for pfs0)
$(BINDIR)/$(OUTPUT).nso: $(BIN)
	@mkdir -p $(BINDIR)
	elf2nso $< $@

# Generate PFS0 (optional)
$(BINDIR)/$(OUTPUT).pfs0: $(BINDIR)/$(OUTPUT).nso
	@mkdir -p $(BINDIR)
	nso2pfs0 $< $@

# Generate final NRO
$(BINDIR)/$(OUTPUT).nro: $(BIN) $(BINDIR)/$(OUTPUT).nacp
	@mkdir -p $(BINDIR)
	nro2nso $< $@ $(NROFLAGS)
