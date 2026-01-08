#---------------------------------------------------------------------------------
# CLEAR THE THE LIST OF SUFFIXES
#---------------------------------------------------------------------------------
.SUFFIXES:

#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

include $(DEVKITPRO)/devkitARM/3ds_rules

# Define tools explicitly to avoid 'ld' not found issues on Windows
PREFIX	:= arm-none-eabi-
export CC		:= $(PREFIX)gcc
export CXX		:= $(PREFIX)g++
export AS		:= $(PREFIX)as
export LD		:= $(PREFIX)gcc
export OBJCOPY	:= $(PREFIX)objcopy
export STRIP	:= $(PREFIX)strip
export NM		:= $(PREFIX)nm
export RANLIB	:= $(PREFIX)ranlib

#---------------------------------------------------------------------------------
# TARGET IS THE NAME OF THE OUTPUT
# BUILD IS THE DIRECTORY WHERE OBJECT FILES ARE PLACED
# SOURCES IS A LIST OF DIRECTORIES CONTAINING SOURCE FILES
# DATA IS A LIST OF DIRECTORIES CONTAINING BINARY DATA
# INCLUDES IS A LIST OF DIRECTORIES CONTAINING HEADER FILES
#---------------------------------------------------------------------------------
TARGET		:=	3dsTok
BUILD		:=	build
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	include

# Paths for devkitPro libraries
LIBCTRU		:=	$(DEVKITPRO)/libctru
LIBCITRO3D	:=	$(DEVKITPRO)/libcitro3d
LIBCITRO2D	:=	$(DEVKITPRO)/libcitro2d

ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -O2 -mword-relocations \
			-fomit-frame-pointer -ffunction-sections \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -D__3DS__

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	:=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) \
			-L$(LIBCTRU)/lib -L$(LIBCITRO3D)/lib -L$(LIBCITRO2D)/lib

LIBS	:=	-lcitro2d -lcitro3d -lctru -lm

#---------------------------------------------------------------------------------
# LIST OF FILES
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

# Include paths relative to project root
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(TOPDIR)/$(dir)) \
					-I$(LIBCTRU)/include \
					-I$(LIBCITRO3D)/include \
					-I$(LIBCITRO2D)/include \
					-I$(TOPDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))

export OFILES	:=	$(BINFILES:.bin=.obj) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)


.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

$(OUTPUT).3dsx	:	$(OUTPUT).elf

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
# RULES FOR BUILDING
#---------------------------------------------------------------------------------
%.o: %.c
	$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) -c $< -o $@

%.o: %.s
	$(AS) -g $(ARCH) -o $@ $<

%.obj: %.bin
	$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
