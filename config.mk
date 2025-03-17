INCS = -Iinc

CPPFLAGS += $(INCS)

OFLAGS = -O3
WFLAGS = -Werror \
		 -pedantic \
		 -Wall \
		 -Wextra \
		 -Wimplicit-fallthrough \
		 -Wold-style-definition \
		 -Wstrict-prototypes \
		 -Wno-unused-parameter
DFLAGS = -g
MFLAGS =
COMPFLAGS = $(WFLAGS) $(OFLAGS) $(DFLAGS) $(MFLAGS)

CFLAGS += $(COMPFLAGS) -std=gnu17 

LDFLAGS += $(MFLAGS)

CC = clang

PKGS += cmocka
PKG_CONFIG ?= pkg-config
CFLAGS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --cflags $(p)))
CXXFLAGS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --libs $(p)))

