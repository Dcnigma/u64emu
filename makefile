ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

export BUILD_EXEFS_SRC := build/exefs

APP_TITLE := kinx
APP_DESCRIPTION := kinx
APP_AUTHOR := MVG & DCNigma
APP_VERSION := 1.0.0
ICON := logo2.jpg

OBJ = obj/global.o obj/2100dasm.o obj/adsp2100.o obj/iMemory.o obj/iMemoryOps.o \
      obj/iBranchOps.o obj/iCPU.o obj/iFPOps.o obj/iATA.o obj/iMain.o obj/hleDSP.o \
      obj/hleMain.o obj/iRom.o obj/EmuObject1.o obj/ki.o obj/iGeneralOps.o \
      obj/mmDisplay.o obj/mmInputDevice.o
LINKOBJ = $(OBJ)

LIBS = -specs=$(DEVKITPRO)/libnx/switch.specs -g \
       -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE \
       -mcpu=cortex-a57+crc+fp+simd \
       -L$(DEVKITPRO)/libnx/lib -L$(DEVKITPRO)/portlibs/switch/lib \
       -lglad -lEGL -lglapi -ldrm_nouveau -lnx

INCS = -I"src/main" -I$(DEVKITPRO)/libnx/include -I$(DEVKITPRO)/portlibs/switch/include
CXXFLAGS = $(INCS) -D__SWITCH__ -march=armv8-a -mcpu=cortex-a57+crc+fp+simd \
           -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections \
           -fno-rtti -fno-exceptions -mtp=soft -fPIE -O3 -w
LINK = aarch64-none-elf-g++
CPP  = aarch64-none-elf-g++
BIN  = release/kinx.elf
BINDIR = release
OUTPUT = kinx
RM = rm -f

# NRO/NACP flags
ifeq ($(strip $(NO_ICON)),)
NROFLAGS += --icon=$(CURDIR)/$(ICON)
endif
ifeq ($(strip $(NO_NACP)),)
NROFLAGS += --nacp=$(BINDIR)/$(OUTPUT).nacp
endif

# ---------------------------------------------------------------------------------
# Main targets
# ---------------------------------------------------------------------------------
.PHONY: all clean

all: $(BINDIR)/$(OUTPUT).nro

# Build ELF
$(BIN): $(OBJ)
	$(LINK) $(LINKOBJ) -o $(BIN) $(LIBS)

# Compile each source file
obj/%.o: %.cpp
	@mkdir -p obj
	$(CPP) -c $< -o $@ $(CXXFLAGS)

# Generate NACP file
$(BINDIR)/$(OUTPUT).nacp:
	@mkdir -p $(BINDIR)
	nx_generate_nacp $(BINDIR)/$(OUTPUT).nacp \
		--title "$(APP_TITLE)" \
		--author "$(APP_AUTHOR)" \
		--version "$(APP_VERSION)" \
		--icon "$(ICON)"

# Build NSO from ELF
$(BINDIR)/$(OUTPUT).nso: $(BIN)
	@mkdir -p $(BINDIR)
	elf2nso $< $@

# Build PFS0 (payload container)
$(BINDIR)/$(OUTPUT).pfs0: $(BINDIR)/$(OUTPUT).nso
	@mkdir -p $(BINDIR)
	nx_mksfo --elf $< --out $@

# Build final NRO
$(BINDIR)/$(OUTPUT).nro: $(BINDIR)/$(OUTPUT).pfs0 $(BINDIR)/$(OUTPUT).nacp
	@mkdir -p $(BINDIR)
	elf2nro $< $@

clean:
	$(RM) $(OBJ) $(BIN) $(BINDIR)/$(OUTPUT).nso \
	       $(BINDIR)/$(OUTPUT).pfs0 $(BINDIR)/$(OUTPUT).nro \
	       $(BINDIR)/$(OUTPUT).nacp
