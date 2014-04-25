########################################
# Build dot2ubi                        #
# By Scott Pakin <scott+d2u@pakin.org> #
########################################

# Don't change this.
VERSION = 1.0

# Adjust the following as desired.
prefix = /usr
bindir = $(prefix)/bin
mandir = $(prefix)/share/man
man1dir = $(mandir)/man1

# The following should work automatically if you have the
# prerequisites installed.  If not, edit as necessary.
CGRAPH_CFLAGS = $(shell pkg-config --cflags libcgraph)
CGRAPH_LDLIBS = $(shell pkg-config --libs libcgraph)
GVC_CFLAGS = $(shell pkg-config --cflags libgvc)
GVC_LDLIBS = $(shell pkg-config --libs libgvc)
XMLRPC_CFLAGS = $(shell xmlrpc-c-config client --cflags)
XMLRPC_LDLIBS = $(shell xmlrpc-c-config client --libs)
UBIGRAPH_CFLAGS =
UBIGRAPH_LDLIBS = -lubigraphclient

# Adjust the following if necessary.
CFLAGS = -Wall -Wextra -g -O2 $(CGRAPH_CFLAGS) $(GVC_CFLAGS) $(UBIGRAPH_CFLAGS) $(XMLRPC_CFLAGS)
LDLIBS = -g -O2 $(CGRAPH_LDLIBS) $(GVC_LDLIBS) $(UBIGRAPH_LDLIBS) $(XMLRPC_LDLIBS)

# ---------------------------------------------------------------------------

DISTFILES = dot2ubi.c dot2ubi.1 README INSTALL LICENSE Makefile

all: dot2ubi

dot2ubi: dot2ubi.o

dot2ubi.o: dot2ubi.c

install: dot2ubi
	install -m 0755 -d $(DESTDIR)$(bindir)
	install -m 0755 dot2ubi $(DESTDIR)$(bindir)/dot2ubi
	install -m 0755 -d $(DESTDIR)$(man1dir)
	install -m 0644 dot2ubi.1 $(DESTDIR)$(man1dir)

clean:
	$(RM) dot2ubi dot2ubi.o

dist: dot2ubi-$(VERSION).tar.gz

dot2ubi-$(VERSION).tar.gz: $(DISTFILES)
	$(RM) -r dot2ubi-$(VERSION)
	install -m 0755 -d dot2ubi-$(VERSION)
	install -m 0644 $(DISTFILES) dot2ubi-$(VERSION)
	tar -czvf dot2ubi-$(VERSION).tar.gz dot2ubi-$(VERSION)

.PHONY: all install clean dist
