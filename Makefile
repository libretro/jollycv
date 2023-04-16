SOURCEDIR := $(abspath $(patsubst %/,%,$(dir $(abspath $(lastword \
	$(MAKEFILE_LIST))))))

AR ?= ar
CC ?= cc
CFLAGS ?= -O2
FLAGS := -std=c11 -Wall -Wextra -Wshadow -Wmissing-prototypes -pedantic
DEPDIR := $(SOURCEDIR)/deps
SRCDIR := $(SOURCEDIR)/src

PKG_CONFIG ?= pkg-config
CFLAGS_JG := $(shell $(PKG_CONFIG) --cflags jg)

INCLUDES := -I$(SRCDIR) -I$(SRCDIR)/z80

NAME := jollycv
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
DATAROOTDIR ?= $(PREFIX)/share
DOCDIR ?= $(DATAROOTDIR)/doc/$(NAME)

BUILD_STATIC ?= 0
USE_VENDORED_SPEEXDSP ?= 0

ifneq ($(BUILD_STATIC), 0)
	LINK = $(strip $(AR) rcs $@ $^ $(LIBS))
	PIC :=
	TARGET := lib$(NAME).a
else
	LINK = $(strip $(CC) $^ $(LDFLAGS) $(LIBS) $(SHARED) -o $@)
	PIC := -fPIC
	SHARED := $(PIC)
	UNAME := $(shell uname -s)
	ifeq ($(UNAME), Darwin)
		SHARED += -dynamiclib
		TARGET := $(NAME).dylib
	else ifeq ($(OS), Windows_NT)
		SHARED += -shared
		TARGET := $(NAME).dll
	else
		SHARED += -shared
		TARGET := $(NAME).so
	endif
endif

CSRCS := z80/z80.c \
	jcv.c \
	jcv_memio.c \
	jcv_mixer.c \
	jcv_psg.c \
	jcv_serial.c \
	jcv_sgmpsg.c \
	jcv_vdp.c \
	jcv_z80.c \
	jg.c

ifneq ($(USE_VENDORED_SPEEXDSP), 0)
	Q_SPEEXDSP :=
	CFLAGS_SPEEXDSP := -I$(DEPDIR)
	LIBS_SPEEXDSP :=
	CSRCS += speex/resample.c
else
	Q_SPEEXDSP := @
	CFLAGS_SPEEXDSP := $(shell $(PKG_CONFIG) --cflags speexdsp)
	LIBS_SPEEXDSP := $(shell $(PKG_CONFIG) --libs speexdsp)
endif

INCLUDES += $(CFLAGS_SPEEXDSP)
LIBS := $(LIBS_SPEEXDSP)

# Object dirs
MKDIRS := speex z80

OBJDIR := objs

# List of object files
OBJS := $(patsubst %,$(OBJDIR)/%,$(CSRCS:.c=.o))

# Compiler command
COMPILE = $(strip $(1) $(CPPFLAGS) $(PIC) $(2) -c $< -o $@)
COMPILE_C = $(call COMPILE, $(CC) $(CFLAGS), $(1))

# Info command
COMPILE_INFO = $(info $(subst $(SOURCEDIR)/,,$(1)))

# Core commands
BUILD_JG = $(call COMPILE_C, $(FLAGS) $(INCLUDES) $(CFLAGS_JG))
BUILD_MAIN = $(call COMPILE_C, $(FLAGS) $(INCLUDES))

.PHONY: all clean install install-strip uninstall

all: $(NAME)/$(TARGET)

# Rules
$(OBJDIR)/%.o: $(DEPDIR)/%.c $(OBJDIR)/.tag
	$(call COMPILE_INFO, $(BUILD_MAIN))
	@$(BUILD_MAIN)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)/.tag
	$(call COMPILE_INFO, $(BUILD_MAIN))
	@$(BUILD_MAIN)

$(OBJDIR)/%.o: $(SOURCEDIR)/%.c $(OBJDIR)/.tag
	$(call COMPILE_INFO, $(BUILD_JG))
	@$(BUILD_JG)

$(OBJDIR)/.tag:
	@mkdir -p -- $(patsubst %,$(OBJDIR)/%,$(MKDIRS))
	@touch $@

$(NAME)/$(TARGET): $(OBJS)
	@mkdir -p $(NAME)
	$(LINK)

clean:
	rm -rf $(OBJDIR) $(NAME)

install: all
	@mkdir -p $(DESTDIR)$(DOCDIR)
	@mkdir -p $(DESTDIR)$(LIBDIR)/jollygood
	cp $(NAME)/$(TARGET) $(DESTDIR)$(LIBDIR)/jollygood/
	cp $(SRCDIR)/z80/LICENSE $(DESTDIR)$(DOCDIR)/LICENSE-z80
	cp $(SOURCEDIR)/LICENSE $(DESTDIR)$(DOCDIR)
	cp $(SOURCEDIR)/README $(DESTDIR)$(DOCDIR)
	$(Q_SPEEXDSP)if test $(USE_VENDORED_SPEEXDSP) != 0; then \
		cp $(DEPDIR)/speex/COPYING \
			$(DESTDIR)$(DOCDIR)/COPYING-speexdsp; \
	fi

install-strip: install
	strip $(DESTDIR)$(LIBDIR)/jollygood/$(TARGET)

uninstall:
	rm -rf $(DESTDIR)$(DOCDIR)
	rm -f $(DESTDIR)$(LIBDIR)/jollygood/$(TARGET)
