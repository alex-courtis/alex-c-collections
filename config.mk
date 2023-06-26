INCS = -Iinc

CPPFLAGS += $(INCS)

OFLAGS = -O3
WFLAGS = -pedantic -Wall -Wextra -Werror -Wno-unused-parameter
DFLAGS = -g
COMPFLAGS = $(WFLAGS) $(OFLAGS) $(DFLAGS)

CFLAGS += $(COMPFLAGS) -std=gnu17 -Wold-style-definition -Wstrict-prototypes

LDFLAGS +=

CC = clang

PKGS += cmocka
PKG_CONFIG ?= pkg-config
CFLAGS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --cflags $(p)))
CXXFLAGS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --libs $(p)))

