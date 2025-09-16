SOURCEDIR := $(abspath $(patsubst %/,%,$(dir $(abspath $(lastword \
	$(MAKEFILE_LIST))))))

NAME := jollycv
JGNAME := $(NAME)

DESCRIPTION := JollyCV is a highly accurate emulator for the ColecoVision \
	(with Super Game Module), CreatiVision (experimental) and My Vision.

SRCDIR := $(SOURCEDIR)/src

INCLUDES = -I$(SRCDIR)/z80 -I$(SRCDIR)/m6502
INCLUDES_JG = -I$(SRCDIR)

LINKER = $(CC)

LIBS =
LIBS_STATIC =

LIBS_REQUIRES := speexdsp

DOCS := ChangeLog LICENSE README

# TODO: add public API header
HEADERS :=

# Object dirs
MKDIRS := m6502 z80

override INSTALL_DATA := 0
override INSTALL_EXAMPLE := 0
override INSTALL_SHARED := 0

include $(SOURCEDIR)/version.h
include $(SOURCEDIR)/mk/jg.mk

INCLUDES += $(CFLAGS_SPEEXDSP)
LIBS += $(LIBS_SPEEXDSP)

EXT := c
FLAGS := -std=c11 $(WARNINGS_DEF_C)

CSRCS := m6502/m6502.c \
	z80/z80.c \
	jcv.c \
	jcv_coleco.c \
	jcv_crvision.c \
	jcv_m6502.c \
	jcv_mixer.c \
	jcv_myvision.c \
	jcv_serial.c \
	jcv_vdp.c \
	jcv_z80.c \
	ay38910.c \
	sn76489.c \
	eep24cxx.c

JGSRCS := jg.c

# List of object files
OBJS := $(patsubst %,$(OBJDIR)/%,$(CSRCS:.c=.o)) $(OBJS_SPEEXDSP)
OBJS_JG := $(patsubst %,$(OBJDIR)/%,$(JGSRCS:.c=.o))

# Core commands
BUILD_JG = $(call COMPILE_C, $(FLAGS) $(INCLUDES_JG) $(CFLAGS_JG))
BUILD_MAIN = $(call COMPILE_C, $(FLAGS) $(INCLUDES))

.PHONY: $(PHONY)

all: $(TARGET)

# Rules
$(OBJDIR)/%.o: $(SRCDIR)/%.$(EXT) $(PREREQ)
	$(call COMPILE_INFO,$(BUILD_MAIN))
	@$(BUILD_MAIN)

# Data rules
install-docs::
	cp $(SRCDIR)/z80/LICENSE $(DESTDIR)$(DOCDIR)/LICENSE-z80

include $(SOURCEDIR)/mk/rules.mk
