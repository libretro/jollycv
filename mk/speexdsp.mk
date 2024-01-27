DIR_SPEEXDSP := $(wildcard $(DEPDIR)/speex)

ifneq ($(DIR_SPEEXDSP),)
	USE_VENDORED_SPEEXDSP ?= 0
else
	override USE_VENDORED_SPEEXDSP := 0
endif

ifneq ($(USE_VENDORED_SPEEXDSP), 0)
	CFLAGS_SPEEXDSP := -I$(DEPDIR)
	LIBS_SPEEXDSP :=  $(if $(findstring -lm,$(LIBS)),,-lm)
	override LIBS_PRIVATE += $(LIBS_SPEEXDSP)
	MKDIRS += deps/speex
	SRCS_SPEEXDSP := deps/speex/resample.c
	OBJS_SPEEXDSP := $(SRCS_SPEEXDSP:.c=.o)

install-docs::
	cp $(DIR_SPEEXDSP)/COPYING $(DESTDIR)$(DOCDIR)/COPYING-speexdsp
else
	override REQUIRES_PRIVATE += speexdsp
	CFLAGS_SPEEXDSP = $(shell $(PKG_CONFIG) --cflags speexdsp)
	LIBS_SPEEXDSP = $(shell $(PKG_CONFIG) --libs speexdsp)
	OBJS_SPEEXDSP :=
endif
