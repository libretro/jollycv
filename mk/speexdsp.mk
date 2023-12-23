USE_VENDORED_SPEEXDSP ?= 0

ifneq ($(USE_VENDORED_SPEEXDSP), 0)
	CFLAGS_SPEEXDSP := -I$(DEPDIR)
	LIBS_SPEEXDSP :=  $(if $(findstring -lm,$(LIBS)),,-lm)
	MKDIRS += deps/speex
	CSRCS += deps/speex/resample.c
else
	override REQUIRES_PRIVATE += speexdsp
	CFLAGS_SPEEXDSP = $(shell $(PKG_CONFIG) --cflags speexdsp)
	LIBS_SPEEXDSP = $(shell $(PKG_CONFIG) --libs speexdsp)
endif
