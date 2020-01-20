# Debian package via cpack
if(NOT CPACK_GENERATOR)
    set(CPACK_GENERATOR "DEB")
endif()
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libmpg123 libsndfile libuv1 libsdl2")
SET(CPACK_PACKAGE_NAME "ury-playd")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "University Radio York") #required
INCLUDE(CPack)
