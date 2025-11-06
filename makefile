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

ifeq ($(strip $(NO_ICON)),)
NROFLAGS += --icon=$(TOPDIR)/$(ICON)
endif
ifeq ($(strip $(NO_NACP)),)
NROFLAGS += --nacp=$(BINDIR)/$(OUTPUT).nacp
endif

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------
.PHONY: all clean all-before all-after clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	$(RM) $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(LINK) $(LINKOBJ) -o $(BIN) $(LIBS)

# Compile all source files
obj/%.o: %.cpp
	@mkdir -p obj
	$(CPP) -c $< -o $@ $(CXXFLAGS)

# ---------------------------------------------------------------------------------
# NRO pipeline (MVG style)
# ---------------------------------------------------------------------------------
all: $(BINDIR)/$(OUTPUT).pfs0 $(BINDIR)/$(OUTPUT).nro

$(BINDIR)/$(OUTPUT).pfs0: $(BINDIR)/$(OUTPUT).nso
$(BINDIR)/$(OUTPUT).nso: $(BIN)

# Only generate NACP if it doesn’t exist
$(BINDIR)/$(OUTPUT).nacp:
	@mkdir -p $(BINDIR)
	if [ ! -f $@ ]; then \
		nx_generate_nacp $@ \
			--title "$(APP_TITLE)" \
			--author "$(APP_AUTHOR)" \
			--version "$(APP_VERSION)" \
			--icon "$(ICON)"; \
	fi

ifeq ($(strip $(NO_NACP)),)
$(BINDIR)/$(OUTPUT).nro: $(BINDIR)/$(OUTPUT).elf $(BINDIR)/$(OUTPUT).nacp
else
$(BINDIR)/$(OUTPUT).nro: $(BINDIR)/$(OUTPUT).elf
endif

$(BINDIR)/$(OUTPUT).elf: $(OBJ)
