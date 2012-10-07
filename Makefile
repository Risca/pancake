CC			?=	gcc
TOPDIR		:=	$(shell pwd)
INCLUDEDIR	:=	$(TOPDIR)/include
COMMONDIR	:=	$(TOPDIR)/common
PORTSDIR	:=	$(TOPDIR)/ports
BUILDDIR	:=	$(TOPDIR)/build
TARGETS		:=	rpi linux

CFLAGS		:=	-I$(INCLUDEDIR)
LDFLAGS		:=

# Define include files
INCLUDES	:=	$(INCLUDEDIR)/pancake.h

# Add some sources
SOURCES		:=	$(COMMONDIR)/pancake.c
SOURCES		+=	$(PORTSDIR)/rpi.c
SOURCES		+=	main.c

OBJECTS		:=	$(SOURCES:.c=.o)

all:

$(TARGETS): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

#$(OBJECTS): $(SOURCES)
#	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJECTS) $(TARGETS)
