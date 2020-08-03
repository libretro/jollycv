CC ?= cc
CFLAGS ?= -O2
FLAGS := -fPIC -std=c99 -Wall -Wextra -Wshadow -Wmissing-prototypes -pedantic
INCLUDES := -Icore -Icore/resample -Icore/z80
SHARED := -fPIC
NAME := jollycv
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib

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

CSRCS := objs/resample/resample.c \
	objs/z80/z80.c \
	objs/jcv.c \
	objs/jcv_memio.c \
	objs/jcv_psg.c \
	objs/jcv_vdp.c \
	objs/jcv_z80.c \
	objs/jg.c

# Object dirs
OBJDIRS := objs objs/resample objs/z80

# List of object files
OBJS := $(CSRCS:.c=.o)

# Rules
objs/%.o: core/%.c
	$(CC) $(CFLAGS) $(FLAGS) $(INCLUDES) -c $< -o $@

objs/%.o: %.c
	$(CC) $(CFLAGS) $(FLAGS) $(INCLUDES) -c $< -o $@

all: maketree $(TARGET)

maketree: $(sort $(OBJDIRS))

$(sort $(OBJDIRS)):
	@mkdir -p $@

$(TARGET): $(OBJS)
	@mkdir -p $(NAME)
	$(CC) $^ $(LDFLAGS) $(SHARED) -o $(NAME)/$(TARGET)

clean:
	rm -rf $(OBJDIRS) $(TARGET) $(NAME)/

install:
	@mkdir -p $(DESTDIR)$(LIBDIR)/jollygood
	cp $(NAME)/$(TARGET) $(DESTDIR)$(LIBDIR)/jollygood/

uninstall:
	rm $(DESTDIR)$(LIBDIR)/jollygood/$(TARGET)
