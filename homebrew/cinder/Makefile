.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET		:=	cinder
BUILD		:=	build
SOURCES		:=	source third_party/imgui
DATA		:=	data
INCLUDES	:=	include third_party/imgui
APP_VERSION	:=	3.50
APP_TITLE	:=	Cinder $(APP_VERSION)
APP_DESCRIPTION	:=	Delete and install software in batches
APP_AUTHOR	:=	Riddle
ICON		:=	icon.png
CIA_NAME	:=	cinder-$(APP_VERSION).cia
DSX_NAME	:=	cinder-$(APP_VERSION).3dsx

ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -O2 -mword-relocations \
			-ffunction-sections \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -D__3DS__

CXXFLAGS	:=	$(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11 \
			-DIMGUI_USER_CONFIG=\"cinder_imconfig.h\"
ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:=	-lcitro2d -lcitro3d -lctru -lm
LIBDIRS	:=	$(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_SOURCES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES		:=	$(OFILES_BIN) $(OFILES_SOURCES)
export HFILES		:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_ICON	:=	$(TOPDIR)/$(ICON)
export _3DSXDEPS	:=	$(OUTPUT).smdh
export _3DSXFLAGS	+=	--smdh=$(CURDIR)/$(TARGET).smdh

.PHONY: all clean cia stamp

all: stamp $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

stamp:
	python3 $(TOPDIR)/resources/stamp_version.py

$(BUILD):
	@mkdir -p $@

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).cia $(CIA_NAME) $(DSX_NAME) resources/banner.bin

cia: all
	python3 $(TOPDIR)/resources/patchheap.py $(OUTPUT).elf
	3dsxtool $(OUTPUT).elf $(TOPDIR)/$(DSX_NAME) $(_3DSXFLAGS)
	python3 $(TOPDIR)/resources/makebanner.py
	makerom -f cia -o $(TOPDIR)/$(CIA_NAME) -elf $(OUTPUT).elf -rsf $(TOPDIR)/resources/app.rsf -icon $(OUTPUT).smdh -banner $(TOPDIR)/resources/banner.bin -target t
	@echo built $(TOPDIR)/$(CIA_NAME)

else

$(OUTPUT).3dsx	:	$(OUTPUT).elf $(_3DSXDEPS)

$(OFILES_SOURCES) : $(HFILES)

$(OUTPUT).elf	:	$(OFILES)

font.o: $(TOPDIR)/resources/pophappiness.ttf

%.bin.o	%_bin.h :	%.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPSDIR)/*.d

endif
