# Created by: Donal Cahill <din@ury.york.ac.uk>
# $FreeBSD$

PORTNAME=		ury-playd
DISTVERSIONPREFIX=	v
DISTVERSION=		0.3.1
CATEGORIES=		audio

MAINTAINER=	din@ury.org.uk
COMMENT=	Minimal C++ audio player

LICENSE=		MIT BSL
LICENSE_COMB=		multi
LICENSE_FILE_MIT=	"${WRKSRC}/LICENSE.txt"
LICENSE_FILE_BSL=	"${WRKSRC}/LICENSE.catch"

LIB_DEPENDS=	libSDL2.so:devel/sdl20 \
		libuv.so:devel/libuv
BUILD_DEPENDS=	cmake:devel/cmake

USES=	cmake compiler:c++14-lang

PLIST_FILES=	bin/playd man/man1/playd.1.gz

USE_GITHUB=	yes
GH_ACCOUNT=	UniversityRadioYork
GH_TAGNAME=	${DISTVERSIONPREFIX}${DISTVERSION}

OPTIONS_MULTI=		LIBS
OPTIONS_MULTI_LIBS=	MPG123 SNDFILE

MPG123_DESC=		"MP3 support"
MPG123_LIB_DEPENDS=	libmpg123.so:audio/mpg123
MPG123_CMAKE_BOOL=	WITH_MPG123

SNDFILE_DESC=		"Ogg Vorbis, WAV, and FLAC support"
SNDFILE_LIB_DEPENDS=	libsndfile.so:audio/libsndfile
SNDFILE_CMAKE_BOOL=	WITH_SNDFILE

OPTIONS_DEFAULT?=	MPG123 SNDFILE

.include <bsd.port.options.mk>

.include <bsd.port.mk>
