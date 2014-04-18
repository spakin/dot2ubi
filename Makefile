########################################
# Build dot2ubi                        #
# By Scott Pakin <scott+d2u@pakin.org> #
########################################

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

CFLAGS = -Wall -Wextra -g -O2 $(CGRAPH_CFLAGS) $(GVC_CFLAGS) $(UBIGRAPH_CFLAGS) $(XMLRPC_CFLAGS)
LDLIBS = -g -O2 $(CGRAPH_LDLIBS) $(GVC_LDLIBS) $(UBIGRAPH_LDLIBS) $(XMLRPC_LDLIBS)

all: dot2ubi

dot2ubi: dot2ubi.o

dot2ubi.o: dot2ubi.c

clean:
	$(RM) dot2ubi dot2ubi.o
