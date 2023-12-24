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

override INSTALL_DATA := 0
override INSTALL_SHARED := 1

include $(SOURCEDIR)/mk/jg.mk

# Object dirs
MKDIRS := z80

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

include $(SOURCEDIR)/mk/speexdsp.mk

INCLUDES += $(CFLAGS_SPEEXDSP)
LIBS += $(LIBS_SPEEXDSP)

# List of object files
OBJS := $(patsubst %,$(OBJDIR)/%,$(CSRCS:.c=.o))
OBJS_JG := $(patsubst %,$(OBJDIR)/%,$(JGSRCS:.c=.o))

# Core commands
BUILD_JG = $(call COMPILE_C, $(FLAGS) $(INCLUDES_JG) $(CFLAGS_JG))
BUILD_MAIN = $(call COMPILE_C, $(FLAGS) $(INCLUDES))

.PHONY: $(PHONY)

all: $(TARGET)

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

$(TARGET_MODULE): $(OBJS_JG) $(OBJS_MODULE)
	@mkdir -p $(NAME)
	$(strip $(CC) -o $@ $< $(LDFLAGS) $(LIBS_MODULE) $(LIBS) $(SHARED))

$(TARGET_SHARED): $(OBJS)
	$(strip $(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(SHARED) $(SONAME))

$(TARGET_STATIC): $(OBJS)
	$(AR) rcs $@ $^

$(TARGET_STATIC_JG): $(OBJS_JG) $(OBJS_SHARED)
	@mkdir -p $(NAME)
	$(AR) rcs $@ $^

$(TARGET_STATIC_MK): $(TARGET_STATIC_JG)
	@printf '%s\n%s\n%s\n%s\n' 'NAME := $(JGNAME)' 'ASSETS :=' \
		'ICONS := $(ICONS_BASE)' 'LIBS_STATIC := $(strip $(LIBS))' > $@

install-docs: all
	@mkdir -p $(DESTDIR)$(DOCDIR)
	cp $(SRCDIR)/z80/LICENSE $(DESTDIR)$(DOCDIR)/LICENSE-z80
	cp $(SOURCEDIR)/LICENSE $(DESTDIR)$(DOCDIR)
	cp $(SOURCEDIR)/README $(DESTDIR)$(DOCDIR)
ifneq ($(USE_VENDORED_SPEEXDSP), 0)
	cp $(DEPDIR)/speex/COPYING $(DESTDIR)$(DOCDIR)/COPYING-speexdsp
endif
