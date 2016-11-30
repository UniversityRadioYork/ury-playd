# This package installs the playd binary and the man page. It can also build and run the tests.
#
# To build on MSYS2:
#
# Open a mingw32 or mingw64 shell.
# Install base-devel, binutils (required for strip), and C and C++ compilers (gcc/clang) for mingw, and git for msys2.
# e.g. $ pacman -S base-devel git mingw-w64-i686-gcc mingw-w64-i686-binutils
#
# Build with `makepkg`
# Useful options: makepkg -s  # install dependencies automatically
#                         -i  # install package once it's built
#                         -c  # clean up pkg/ and src/ after building
#                         -f  # force build if package already exists
#                  --nocheck  # skip building and running the tests
#
# See `man makepkg` for more

_basename=ury-playd
_realname=${_basename}-git
pkgver=git
pkgrel=1
pkgdesc="ury-playd description"
url="https://universityradioyork.github.io/ury-playd/"
license=("MIT" "custom")
arch=("i686" "x86_64")

pkgname="${MINGW_PACKAGE_PREFIX}-${_realname}"

makedepends=("${MINGW_PACKAGE_PREFIX}-cmake"
             "${MINGW_PACKAGE_PREFIX}-libuv"
		     "${MINGW_PACKAGE_PREFIX}-mpg123"
		     "${MINGW_PACKAGE_PREFIX}-libsndfile"
		     "${MINGW_PACKAGE_PREFIX}-SDL2")
depends=("${MINGW_PACKAGE_PREFIX}-libuv"
		 "${MINGW_PACKAGE_PREFIX}-mpg123"
		 "${MINGW_PACKAGE_PREFIX}-libsndfile"
		 "${MINGW_PACKAGE_PREFIX}-SDL2")

provides=("${MINGW_PACKAGE_PREFIX}-ury-playd")
conflicts=("${MINGW_PACKAGE_PREFIX}-ury-playd")

source=("ury-playd-git::git+http://github.com/UniversityRadioYork/ury-playd.git#branch=mingw")
md5sums=("SKIP")

pkgver() {
  cd "${srcdir}/${_realname}"
  git describe --long --tags | sed 's/\([^-]*-g\)/r\1/;s/-/./g'
}

build() {
  cd "${srcdir}/${_realname}"
  cmake -G "Unix Makefiles" "${srcdir}/${_realname}"
  make playd \
    MINGW_TARGET=${MINGW_CHOST}
}

package() {
  cd "${srcdir}/${_realname}"
  cmake -G "Unix Makefiles" "${srcdir}/${_realname}" \
    -DCMAKE_INSTALL_PREFIX:PATH="${pkgdir}${MINGW_PREFIX}/"
  make install \
    MINGW_TARGET=${MINGW_CHOST}
}

check() {
  cd "${srcdir}/${_realname}"
  cmake -G "Unix Makefiles" "${srcdir}/${_realname}"
  make check \
    MINGW_TARGET=${MINGW_CHOST}
}
