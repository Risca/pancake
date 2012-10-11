CC			:=	gcc
LD			:=	gcc
RM			:=	rm -rf
TOPDIR		:=	$(shell pwd)
INCLUDEDIR	:=	$(TOPDIR)/include
COMMONDIR	:=	$(TOPDIR)/common
PORTSDIR	:=	$(TOPDIR)/ports
BUILDDIR	:=	$(TOPDIR)/build
TARGETS		:=	rpi linux
TARGET		:=	$(filter $(MAKECMDGOALS),$(TARGETS))
VPATH		:=	$(COMMONDIR) $(PORTSDIR)

CFLAGS		:=	-I$(INCLUDEDIR)
LDFLAGS		:=

# Define include files
INCLUDES	:=	$(INCLUDEDIR)/pancake.h

# Add some sources
SOURCES		:=	$(COMMONDIR)/pancake.c
SOURCES		+=	$(PORTSDIR)/$(TARGET).c
SOURCES		+=	$(TOPDIR)/main.c

# A bit messy
OBJECTS		:=	$(patsubst %.c,%.o,$(addprefix $(BUILDDIR)/, $(notdir $(SOURCES))))

# For quiet builds
ifndef V
$(foreach VAR,CC CXX LD AR RANLIB RC,\
    $(eval override $(VAR) = @printf " %s\t%s\n" $(VAR) "$$(notdir $$@)"; $($(VAR))))
endif

# Ok, let's go!
ifneq ($(TARGET),)
$(info Building for $(TARGET))
endif

all:

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $(BUILDDIR)/$@ $^

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(OBJECTS) $(TARGETS)
