CC			:=	gcc
LD			:=	gcc
RM			:=	rm -rf
TOPDIR		:=	$(shell pwd)
INCLUDEDIR	+=	$(TOPDIR)/include
COMMONDIR	:=	$(TOPDIR)/common
PORTSDIR	:=	$(TOPDIR)/ports
BUILDDIR	:=	$(TOPDIR)/build
TARGETS		:=	rpi linux linux_sockets
TARGET		:=	$(filter $(MAKECMDGOALS),$(TARGETS))
VPATH		:=	$(COMMONDIR) $(PORTSDIR)

# Add some sources
SOURCES		:=	$(COMMONDIR)/pancake.c
SOURCES		+=	$(COMMONDIR)/in6_addr.c
SOURCES		+=	$(PORTSDIR)/$(TARGET).c
SOURCES		+=	$(TOPDIR)/main.c

# Define include files
INCLUDES	:=	$(INCLUDEDIR)/pancake.h

# Include target specific stuff
ifneq ($(TARGET),)
$(info Building for $(TARGET))
include Makefile.$(TARGET)
endif

# Make sure this happens after target specific include
CFLAGS		+=	-I$(INCLUDEDIR) -Wall -Wno-missing-braces
LDFLAGS		+=	-Wall -Wno-missing-braces

# A bit messy
OBJECTS		:=	$(patsubst %.c,%.o,$(addprefix $(BUILDDIR)/, $(notdir $(SOURCES))))

# For quiet builds
ifndef V
$(foreach VAR,CC CXX LD AR RANLIB RC,\
    $(eval override $(VAR) = @printf " %s\t%s\n" $(VAR) "$$(notdir $$@)"; $($(VAR))))
endif

# Ok, let's go!
all:
	$(error You have to specify a target! Available targets: $(TARGETS))

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $(BUILDDIR)/$@ $^

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJECTS): | $(BUILDDIR)

$(BUILDDIR):
	@test -d $(BUILDDIR) || mkdir $(BUILDDIR)

clean:
	$(RM) $(OBJECTS) $(TARGETS)

distclean: clean
	rm -rf $(BUILDDIR)
