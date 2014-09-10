# Basic *BSD Makefile.
#
# For more bells and whistles, use the GNU Makefile (`Makefile`).

PROG		=	playd
SRCS		!= 	find src -name "*.cpp" -or -name "*.c"
MAN		=	src/playd.1

PKGS		=	portaudio-2.0 sox libuv
CFLAGS		+=	`pkgconf --cflags $(PKGS)` --std=c99
CXXFLAGS	+=	`pkgconf --cflags $(PKGS)` --std=c++11 -Isrc/contrib
LDFLAGS		+=	`pkgconf --libs $(PKGS)`

.include <bsd.prog.mk>

# Make sure the targets go where the Makefile expects them.
# Otherwise, CC/CXX will dump the output file in the current working directory.

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC} -o ${.TARGET}

.cpp.o:
	${CXX} ${CXXFLAGS} -c ${.IMPSRC} -o ${.TARGET}
