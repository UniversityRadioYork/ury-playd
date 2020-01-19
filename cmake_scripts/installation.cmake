# Binary
install(TARGETS playd RUNTIME DESTINATION bin)

# Manpages
set(mandir "man/man1")
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL FreeBSD)
  set(mandir "share/${mandir}")
endif()
install(FILES src/playd.1       DESTINATION ${CMAKE_INSTALL_PREFIX}/${mandir})

# Licenses
install(FILES LICENSE.txt       DESTINATION ${CMAKE_INSTALL_PREFIX}/share/licenses/ury-playd)
install(FILES LICENSE.catch     DESTINATION ${CMAKE_INSTALL_PREFIX}/share/licenses/ury-playd)
