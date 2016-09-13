cmake_minimum_required(VERSION 2.8)
project(playd)
list(APPEND CMAKE_MODULE_PATH "${playd_SOURCE_DIR}/cmake_scripts")

list(APPEND CMAKE_PREFIX_PATH "${playd_SOURCE_DIR}")
include(set_compiler_flags)

find_package(SDL2 REQUIRED)
find_package(LibUV REQUIRED)

# Declare formats provided by each lib
set(MPG123_FMTS MP3)
set(SNDFILE_FMTS OGG WAV FLAC)

set(SUPPORTED_FORMATS)
foreach(loop_var MPG123 SNDFILE)
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
  if(WIN32)
    add_definitions(-DLINK_MPG123_DLL)
  endif()
  set(SRCS ${SRCS} src/audio/sources/mp3.cpp)
else()
  add_definitions(-DNO_MP3)
endif()

# Def if sndfile found
if(SNDFILE_FOUND)
  add_definitions(-DWITH_SNDFILE)
  set(SRCS ${SRCS} src/audio/sources/sndfile.cpp)
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

set(SRCS ${SRCS}
  src/errors.cpp
  src/io.cpp
  src/player.cpp
  src/response.cpp
  src/tokeniser.cpp
  src/audio/audio.cpp
  src/audio/audio_sink.cpp
  src/audio/audio_source.cpp
  src/audio/ringbuffer.cpp
  src/audio/sample_formats.cpp
  src/contrib/pa_ringbuffer/pa_ringbuffer.c
  )
set(tests_SRCS ${tests_SRCS}
  src/tests/dummy_audio_sink.cpp
  src/tests/dummy_audio_source.cpp
  src/tests/dummy_response_sink.cpp
  src/tests/errors.cpp
  src/tests/io_response.cpp
  src/tests/main.cpp
  src/tests/no_audio.cpp
  src/tests/pipe_audio.cpp
  src/tests/player.cpp
  src/tests/ringbuffer.cpp
  src/tests/tokeniser.cpp
)
add_executable(playd ${SRCS} "src/main.cpp")
add_executable(playd_tests ${SRCS} ${tests_SRCS})
add_test(NAME playd_tests COMMAND playd_tests)
# `make check` to both compile and run tests
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
  DEPENDS playd_tests)
enable_testing()

foreach(mylib SDL2 LIBUV MPG123 SNDFILE)
  if(${mylib}_LIBRARY)
    set(libs ${mylib}_LIBRARY)
  elseif(${mylib}_LIBRARIES)
    set(libs ${mylib}_LIBRARIES)
  endif()
  if(libs)
	message(STATUS "Linking: ${libs} ${${libs}}")
    target_link_libraries(playd ${${libs}})
	target_link_libraries(playd_tests ${${libs}})
	include_directories(playd ${${mylib}_INCLUDE_DIR})
	include_directories(playd_tests ${${mylib}_INCLUDE_DIR})
  endif()
  unset(libs)
endforeach()
