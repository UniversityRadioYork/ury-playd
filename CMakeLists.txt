cmake_minimum_required(VERSION 3.4)
project(playd)

list(APPEND CMAKE_MODULE_PATH "${playd_SOURCE_DIR}/cmake_scripts")

include(add_sources)
include(set_compiler_flags)

find_package(SDL2 REQUIRED)
find_package(LibUV REQUIRED)

# Declare formats provided by each lib
set(MPG123_FMTS MP3)
set(LIBSNDFILE_FMTS OGG WAV FLAC)

set(SUPPORTED_FORMATS)
foreach(loop_var MPG123 LIBSNDFILE)
  find_package(${loop_var})
  if(${loop_var}_FOUND)
    list(APPEND SUPPORTED_FMTS ${${loop_var}_FMTS})
  endif(${loop_var}_FOUND)
endforeach()

if("${SUPPORTED_FMTS}" STREQUAL "")
  message(FATAL_ERROR "No audio format libraries could be found")
endif()
list(REMOVE_DUPLICATES SUPPORTED_FMTS)
message(STATUS "Supported formats: ${SUPPORTED_FMTS}")

# Def if MP3 supported
list (FIND SUPPORTED_FMTS "MP3" _index)
if (${_index} GREATER -1)
  add_definitions(-DWITH_MP3)
else()
  add_definitions(-DNO_MP3)
endif()

# Def if sndfile found
if(LIBSNDFILE_FOUND)
  add_definitions(-DWITH_SNDFILE)
else()
  add_definitions(-DNO_SNDFILE)
endif()

# TODO: Manual versioning?
execute_process(COMMAND "git" "describe" "--tags" "--always" WORKING_DIRECTORY "${playd_SOURCE_DIR}" RESULT_VARIABLE NOT_GIT_REPO OUTPUT_VARIABLE PROGVER OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT_GIT_REPO)
  message(FATAL "Not a git repo")
else()
  add_definitions(-DPD_VERSION="${PROGVER}")
endif()

##

include_directories(${SDL2_INCLUDE_DIR} ${LIBUV_INCLUDE_DIR} ${MPG123_INCLUDE_DIR} ${LIBSNDFILE_INCLUDE_DIR})

add_subdirectory("src")

get_property(SRCS GLOBAL PROPERTY SRCS_LIST)
add_executable(playd ${SRCS})
target_link_libraries(playd ${SDL2_LIBRARY} ${LIBUV_LIBRARY} ${MPG123_LIBRARY} ${LIBSNDFILE_LIBRARY})

# Print all variables
get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
