SOURCEDIR := $(abspath $(patsubst %/,%,$(dir $(abspath $(lastword \
	$(MAKEFILE_LIST))))))

NAME := jollycv
JGNAME := $(NAME)

DESCRIPTION := JollyCV is a highly accurate emulator for the ColecoVision, \
	including support for the Super Game Module.

# https://semver.org/
VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 1

CFLAGS ?= -O2

FLAGS := -std=c11 -Wall -Wextra -Wshadow -Wmissing-prototypes -pedantic
DEPDIR := $(SOURCEDIR)/deps
SRCDIR := $(SOURCEDIR)/src

INCLUDES := -I$(SRCDIR)/z80
INCLUDES_JG := -I$(SRCDIR)

LIBS :=

DOCS := LICENSE README

# Object dirs
MKDIRS := z80

override INSTALL_DATA := 0
override INSTALL_SHARED := 1

include $(SOURCEDIR)/mk/jg.mk
include $(SOURCEDIR)/mk/speexdsp.mk

INCLUDES += $(CFLAGS_SPEEXDSP)
LIBS += $(LIBS_SPEEXDSP)

LINKER := $(CC)

CSRCS := z80/z80.c \
	jcv.c \
	jcv_memio.c \
	jcv_mixer.c \
	jcv_psg.c \
	jcv_serial.c \
	jcv_sgmpsg.c \
	jcv_vdp.c \
	jcv_z80.c

JGSRCS := jg.c

# List of object files
OBJS := $(patsubst %,$(OBJDIR)/%,$(CSRCS:.c=.o) $(OBJS_SPEEXDSP))
OBJS_JG := $(patsubst %,$(OBJDIR)/%,$(JGSRCS:.c=.o))

# Core commands
BUILD_JG = $(call COMPILE_C, $(FLAGS) $(INCLUDES_JG) $(CFLAGS_JG))
BUILD_MAIN = $(call COMPILE_C, $(FLAGS) $(INCLUDES))

.PHONY: $(PHONY)

all: $(TARGET)

include $(SOURCEDIR)/mk/rules.mk

# Rules
$(OBJDIR)/%.o: $(DEPDIR)/%.c $(OBJDIR)/.tag
	$(call COMPILE_INFO,$(BUILD_MAIN))
	@$(BUILD_MAIN)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)/.tag
	$(call COMPILE_INFO,$(BUILD_MAIN))
	@$(BUILD_MAIN)

# Shim rules
$(OBJDIR)/%.o: $(SOURCEDIR)/%.c $(OBJDIR)/.tag
	$(call COMPILE_INFO,$(BUILD_JG))
	@$(BUILD_JG)

install-docs::
	cp $(SRCDIR)/z80/LICENSE $(DESTDIR)$(DOCDIR)/LICENSE-z80
