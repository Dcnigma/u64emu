
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)

include $(DEVKITPRO)/libnx/switch_rules

export BUILD_EXEFS_SRC := build/exefs

APP_TITLE := kinx
APP_DESCRIPTION := kinx
APP_AUTHOR := MVG and DcNigma
APP_VERSION := 1.0.0
ICON := logo2.jpg

WINDRES   = windres.exe
OBJ       = obj/2100dasm.o obj/adsp2100.o obj/iMemory.o obj/iMemoryOps.o obj/iBranchOps.o obj/iCPU.o obj/iFPOps.o obj/iATA.o obj/iMain.o obj/hleDSP.o obj/hleMain.o obj/iRom.o obj/CEmuObject.o obj/ki.o obj/iGeneralOps.o obj/mmDisplay.o obj/mmInputDevice.o
LINKOBJ   = $(OBJ)
LIBS      = -specs=$(DEVKITPRO)/libnx/switch.specs -g -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE -mcpu=cortex-a57+crc+fp+simd -L$(DEVKITPRO)/libnx/lib -L$(DEVKITPRO)/portlibs/switch/lib -lglad -lEGL -lglapi -ldrm_nouveau -lnx
INCS      = -I"src/main" -I$(DEVKITPRO)/libnx/include -I$(DEVKITPRO)/portlibs/switch/include
CXXINCS   = $(INCS)
BIN       = release/kinx.elf
BUILD	  =	build
BINDIR	  = release
DEFINES   = -D__SWITCH__
CXXFLAGS  = $(CXXINCS) $(DEFINES) -march=armv8-a -mcpu=cortex-a57+crc+fp+simd -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections -fno-rtti -fno-exceptions -mtp=soft -fPIE -O3 -w
CFLAGS    = $(INCS) $(DEFINES)    -march=armv8-a -mcpu=cortex-a57+crc+fp+simd -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections -fno-rtti -fno-exceptions -mtp=soft -fPIE -O3 -w
RM        = rm -f
LINK      = aarch64-none-elf-g++
CPP		  = aarch64-none-elf-g++
OUTPUT    = kinx

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(BINDIR)/$(OUTPUT).nacp
endif

ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

.PHONY: all all-before all-after clean clean-custom
all: all-before $(BIN) all-after

clean: clean-custom
	$(RM) $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(LINK) $(LINKOBJ) -o $(BIN) $(LIBS)

# Compile each source
#done
obj/2100dasm.o: 2100dasm.cpp
	$(CPP) -c 2100dasm.cpp -o obj/2100dasm.o $(CXXFLAGS)

obj/adsp2100.o: adsp2100.cpp
	$(CPP) -c adsp2100.cpp -o obj/adsp2100.o $(CXXFLAGS)
#done
obj/iMemory.o: iMemory.cpp
	$(CPP) -c iMemory.cpp -o obj/iMemory.o $(CXXFLAGS)
#done
obj/iMemoryOps.o: iMemoryOps.cpp
	$(CPP) -c iMemoryOps.cpp -o obj/iMemoryOps.o $(CXXFLAGS)
#done
obj/iBranchOps.o: iBranchOps.cpp
	$(CPP) -c iBranchOps.cpp -o obj/iBranchOps.o $(CXXFLAGS)
#done
obj/iCPU.o: iCPU.cpp
	$(CPP) -c iCPU.cpp -o obj/iCPU.o $(CXXFLAGS)
#done
obj/iFPOps.o: iFPOps.cpp
	$(CPP) -c iFPOps.cpp -o obj/iFPOps.o $(CXXFLAGS)
#done
obj/iATA.o: iATA.cpp
	$(CPP) -c iATA.cpp -o obj/iATA.o $(CXXFLAGS)
#done
obj/iMain.o: iMain.cpp
	$(CPP) -c iMain.cpp -o obj/iMain.o $(CXXFLAGS)
#done
obj/hleDSP.o: hleDSP.cpp
	$(CPP) -c hleDSP.cpp -o obj/hleDSP.o $(CXXFLAGS)
#done
obj/hleMain.o: hleMain.cpp
	$(CPP) -c hleMain.cpp -o obj/hleMain.o $(CXXFLAGS)
#done
obj/iRom.o: iRom.cpp
	$(CPP) -c iRom.cpp -o obj/iRom.o $(CXXFLAGS)
#done
obj/CEmuObject.o: EmuObject.cpp
	$(CPP) -c EmuObject.cpp -o obj/EmuObject.o $(CXXFLAGS)
#done
obj/ki.o: ki.cpp
	$(CPP) -c ki.cpp -o obj/ki.o $(CXXFLAGS)
#done
obj/iGeneralOps.o: iGeneralOps.cpp
	$(CPP) -c iGeneralOps.cpp -o obj/iGeneralOps.o $(CXXFLAGS)
#done
obj/mmDisplay.o: mmDisplay.cpp
	$(CPP) -c mmDisplay.cpp -o obj/mmDisplay.o $(CXXFLAGS)
#done
obj/mmInputDevice.o: mmInputDevice.cpp
	$(CPP) -c mmInputDevice.cpp -o obj/mmInputDevice.o $(CXXFLAGS)
obj/mmInputDevice.o: $(GLOBALDEPS) mmInputDevice.cpp
	$(CPP) -c mmInputDevice.cpp -o obj/mmInputDevice.o $(CXXFLAGS)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(BINDIR)/$(OUTPUT).pfs0 $(BINDIR)/$(OUTPUT).nro

$(BINDIR)/$(OUTPUT).pfs0	:	$(BINDIR)/$(OUTPUT).nso

$(BINDIR)/$(OUTPUT).nso	:	$(BINDIR)/$(OUTPUT).elf

ifeq ($(strip $(NO_NACP)),)
$(BINDIR)/$(OUTPUT).nro	:	$(BINDIR)/$(OUTPUT).elf $(BINDIR)/$(OUTPUT).nacp
else
$(BINDIR)/$(OUTPUT).nro	:	$(BINDIR)/$(OUTPUT).elf
endif

$(BINDIR)/$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

# end of Makefile ...
