## BEGIN USER-CHANGEABLE VARIABLES ##

# The name of the outputted binary.
NAME = playslave++

# Where to put the object files.
OBJDIR ?= build

# Where to put the finished binary.
BINDIR ?= build

# Where the source resides.
SRCDIR ?= .

# The warning flags to use when building playslav e++.
WARNS ?= -Wall -Wextra -Werror -pedantic

# Programs used during building.
PKG_CONFIG ?= pkg-config
CC         ?= gcc
CXX        ?= g++

## END USER-CHANGEABLE VARIABLES ##


# Calculate the path to the outputted file.
BIN = $(BINDIR)/playslave++

# This should include all of the source directories for playslave++,
# excluding any special ones defined below.  The root source directory is
# implied.
SUBDIRS = audio io player contrib/pa_ringbuffer

# Now we work out which libraries to use, using pkg-config.
# These packages are always used: the PortAudio C library, and FFmpeg.
PKGS = portaudio-2.0 libavcodec libavformat libavutil libswresample

# PortAudio's C++ bindings aren't always available, so we bundle them.
# However, if there is a PortAudioCPP package available, we can make use of it
# instead.
HAS_PORTAUDIOCPP ?= $(shell $(PKG_CONFIG) --list | grep 'portaudiocpp')
ifeq "$(HAS_PORTAUDIOCPP)" ""
  PORTAUDIOCPP_DIR  = contrib/portaudiocpp
  CXXFLAGS         += -I"$(SRCDIR)/$(PORTAUDIOCPP_DIR)"
  SUBDIRS          += $(PORTAUDIOCPP_DIR)
else
  PKGS += portaudiocpp
endif

# Set up the flags needed to use the packages.
PKG_CFLAGS  += `$(PKG_CONFIG) --cflags $(PKGS)`
PKG_LDFLAGS += `$(PKG_CONFIG) --libs $(PKGS)`

# Now we make up the source and object directory sets...
SRC_SUBDIRS = $(SRCDIR) $(addprefix $(SRCDIR)/,$(SUBDIRS))
OBJ_SUBDIRS = $(OBJDIR) $(addprefix $(OBJDIR)/,$(SUBDIRS))

# ...And find the sources to compile and the objects they make.
SOURCES  = $(foreach dir,$(SRC_SUBDIRS),$(wildcard $(dir)/*.cpp))
OBJECTS  = $(patsubst $(SRCDIR)%,$(OBJDIR)%,$(SOURCES:.cpp=.o))
CSOURCES = $(foreach dir,$(SRC_SUBDIRS),$(wildcard $(dir)/*.c))
COBJECTS = $(patsubst $(SRCDIR)%,$(OBJDIR)%,$(CSOURCES:.c=.o))

# Now set up the flags needed for playslave++.
CFLAGS   += -c $(WARNS) $(PKG_CFLAGS) -g -std=c99
CXXFLAGS += -c $(WARNS) $(PKG_CFLAGS) -g -std=c++11
LDFLAGS  += $(PKG_LDFLAGS) -lboost_system

## BEGIN RULES ##

all: mkdir $(BIN)

$(BIN): $(COBJECTS) $(OBJECTS)
	@echo LINK $@
	@$(CXX) $(COBJECTS) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo CC $@
	@$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo CXX $@
	@$(CXX) $(CXXFLAGS) $< -o $@

clean:
	@echo CLEAN
	@rm -f $(OBJECTS) $(COBJECTS) $(BIN)

mkdir:
	@echo MKDIR
	@mkdir -p $(OBJ_SUBDIRS)
	@mkdir -p $(BINDIR)

run: $(BIN)
	./$(BIN)

gdbrun: $(BIN)
	gdb $(BIN)
