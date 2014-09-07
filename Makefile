## BEGIN USER-CHANGEABLE VARIABLES ##

# Where to put the object files and other intermediate fluff.
builddir ?= build

# Where the source resides.
srcdir ?= src

# The warning flags to use when building playd.
WARNS ?= -Wall -Wextra -Werror

# Programs used during building.
CC         ?= clang
CXX        ?= clang++
GZIP       ?= gzip -9 --stdout
INSTALL    ?= install
PKG_CONFIG ?= pkg-config
FORMAT     ?= clang-format -i
DOXYGEN    ?= doxygen
GIT        ?= git

# Variables used to decide where to install playd and its man pages.
prefix      ?= /usr/local
bindir      ?= $(prefix)/bin
mandir      ?= /usr/share/man/man1

## END USER-CHANGEABLE VARIABLES ##

# The name of the program.  This is hardcoded in several places, such as the
# man page, so users are not recommended to change it.
NAME = playd

# Calculate the path to the outputted program.
BIN = $(builddir)/$(NAME)

MAN_SRC = $(srcdir)/$(NAME).1
MAN_GZ  = $(builddir)/$(NAME).1.gz

# This should include all of the source directories for playd,
# excluding any special ones defined below.  The root source directory is
# implied.
OWN_SUBDIRS = audio io player
SUBDIRS     = $(OWN_SUBDIRS) contrib/pa_ringbuffer

# Now we work out which libraries to use, using pkg-config.
# These packages are always used: the PortAudio C library, and FFmpeg.
PKGS = portaudio-2.0 sox libuv

# PortAudio's C++ bindings aren't always available, so we bundle them.
# However, if there is a PortAudioCPP package available, we can make use of it
# instead.
HAS_PORTAUDIOCPP ?= $(shell $(PKG_CONFIG) --list-all | grep 'portaudiocpp')
ifeq "$(HAS_PORTAUDIOCPP)" ""
  PORTAUDIOCPP_DIR  = contrib/portaudiocpp
  CXXFLAGS         += -I"$(srcdir)/$(PORTAUDIOCPP_DIR)"
  SUBDIRS          += $(PORTAUDIOCPP_DIR)
else
  PKGS += portaudiocpp
endif

# Set up the flags needed to use the packages.
PKG_CFLAGS  += `$(PKG_CONFIG) --cflags $(PKGS)`
PKG_LDFLAGS += `$(PKG_CONFIG) --libs $(PKGS)`

# Now we make up the source and object directory sets...
SRC_SUBDIRS = $(srcdir) $(addprefix $(srcdir)/,$(SUBDIRS))
OBJ_SUBDIRS = $(builddir) $(addprefix $(builddir)/,$(SUBDIRS))

# ...And find the sources to compile and the objects they make.
SOURCES  = $(foreach dir,$(SRC_SUBDIRS),$(wildcard $(dir)/*.cpp))
OBJECTS  = $(patsubst $(srcdir)%,$(builddir)%,$(SOURCES:.cpp=.o))
CSOURCES = $(foreach dir,$(SRC_SUBDIRS),$(wildcard $(dir)/*.c))
COBJECTS = $(patsubst $(srcdir)%,$(builddir)%,$(CSOURCES:.c=.o))

# These are used for source transformations, such as formatting.
# We don't want to disturb contributed source with these.
OWN_SRC_SUBDIRS = $(srcdir) $(addprefix $(srcdir)/,$(OWN_SUBDIRS))
OWN_SOURCES     = $(foreach dir,$(OWN_SRC_SUBDIRS),$(wildcard $(dir)/*.cpp))
OWN_CSOURCES    = $(foreach dir,$(OWN_SRC_SUBDIRS),$(wildcard $(dir)/*.c))
OWN_HEADERS     = $(foreach dir,$(OWN_SRC_SUBDIRS),$(wildcard $(dir)/*.hpp))
OWN_CHEADERS    = $(foreach dir,$(OWN_SRC_SUBDIRS),$(wildcard $(dir)/*.h))
TO_FORMAT       = $(OWN_SOURCES) $(OWN_CSOURCES) $(OWN_HEADERS) $(OWN_CHEADERS)

# Now set up the flags needed for playd.
CFLAGS   += -c $(WARNS) $(PKG_CFLAGS) -g -std=c99
CXXFLAGS += -c $(WARNS) $(PKG_CFLAGS) -g -std=c++11
LDFLAGS  += $(PKG_LDFLAGS)

## BEGIN RULES ##

.PHONY: clean mkdir install run gdbrun format gh-pages doc

all: mkdir $(BIN) man
man: $(MAN_GZ)

$(BIN): $(COBJECTS) $(OBJECTS)
	@echo LINK $@
	@$(CXX) $(COBJECTS) $(OBJECTS) $(LDFLAGS) -o $@

$(builddir)/%.o: $(srcdir)/%.c
	@echo CC $@
	@$(CC) $(CFLAGS) $< -o $@

$(builddir)/%.o: $(srcdir)/%.cpp
	@echo CXX $@
	@$(CXX) $(CXXFLAGS) $< -o $@

$(builddir)/%.1.gz: $(srcdir)/%.1
	@echo GZIP $@
	@< $< ${GZIP} > $@

clean:
	@echo CLEAN
	@rm -f $(OBJECTS) $(COBJECTS) $(MAN_GZ) $(BIN)

mkdir:
	mkdir -p $(OBJ_SUBDIRS)

install: $(BIN) $(MAN_GZ)
	$(INSTALL) $(BIN) $(bindir)
	-$(INSTALL) $(MAN_GZ) $(mandir)

run: $(BIN)
	./$(BIN)

gdbrun: $(BIN)
	gdb $(BIN)

format: $(TO_FORMAT)
	@echo FORMAT $^
	@$(FORMAT) $^

gh-pages: doc
	git checkout gh-pages
	mv doc/html doxygen
	git add doxygen
	git commit -m "Update doxygen on gh-pages."

doc:
	${DOXYGEN}
