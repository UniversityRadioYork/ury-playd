# function to collect all the sources from sub-directories
# into a single list
function(add_sources binary)
  get_property(is_defined GLOBAL PROPERTY ${binary}_SRCS DEFINED)
  if(NOT is_defined)
    define_property(GLOBAL PROPERTY ${binary}_SRCS
      BRIEF_DOCS "List of source files"
      FULL_DOCS "List of source files to be compiled in one library")
  endif()
  # make absolute paths
  set(SRCS)
  foreach(s IN LISTS ARGN)
    if(NOT IS_ABSOLUTE "${s}")
      get_filename_component(s "${s}" ABSOLUTE)
    endif()
    list(APPEND SRCS "${s}")
  endforeach()
  # append to global list
  set_property(GLOBAL APPEND PROPERTY ${binary}_SRCS "${SRCS}")
endfunction(add_sources)
