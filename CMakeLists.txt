cmake_minimum_required(VERSION 3.11) # for workgroups

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_scripts")

# Set version from git tag
include(version)

project(playd VERSION "${PROJECTVER}")

# Debian package via cpack
include(set_cpack_flags)

# Try to build with mpg123 and sndfile by default
# Override on the CLI: `cmake -DWITH_MPG123=OFF`
option(WITH_MPG123 "Enable MPG123 support" ON)
option(WITH_SNDFILE "Enable libsndfile support" ON)

# Add in the location of the GSL, which expects itself to be in a system include directory
include_directories("${playd_SOURCE_DIR}/src/contrib/GSL/include")

# Find mandatory libraries
find_package(SDL2 REQUIRED)
find_package(LIBUV REQUIRED)

# Declare formats provided by each lib
set(MPG123_FMTS MP3)
set(SNDFILE_FMTS OGG WAV FLAC)

# Find desired libraries, and add the formats they provide to SUPPORTED_FORMATS
set(SUPPORTED_FORMATS)
foreach(loop_var MPG123 SNDFILE)
  if(WITH_${loop_var})
    find_package(${loop_var})
    if(${loop_var}_FOUND)
      list(APPEND SUPPORTED_FMTS ${${loop_var}_FMTS})
    endif()
  endif()
endforeach()

if("${SUPPORTED_FMTS}" STREQUAL "")
  message(FATAL_ERROR "No audio format libraries could be found")
endif()
list(REMOVE_DUPLICATES SUPPORTED_FMTS)
message(STATUS "Supported formats: ${SUPPORTED_FMTS}")

# Def if MP3 supported
list(FIND SUPPORTED_FMTS "MP3" _index)
if(${_index} GREATER -1)
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

# Add sources
set(SRCS ${SRCS}
  src/errors.cpp
  src/io.cpp
  src/player.cpp
  src/response.cpp
  src/tokeniser.cpp
  src/audio/audio.cpp
  src/audio/sink.cpp
  src/audio/source.cpp
  src/audio/ringbuffer.cpp
  src/audio/sample_format.cpp
  )
set(tests_SRCS ${tests_SRCS}
  src/tests/dummy_audio_sink.cpp
  src/tests/dummy_audio_source.cpp
  src/tests/dummy_response_sink.cpp
  src/tests/errors.cpp
  src/tests/response.cpp
  src/tests/main.cpp
  src/tests/null_audio.cpp
  src/tests/basic_audio.cpp
  src/tests/player.cpp
  src/tests/ringbuffer.cpp
  src/tests/tokeniser.cpp
)
add_executable(playd ${SRCS} "src/main.cpp")
target_compile_features(playd PUBLIC cxx_std_17)

add_executable(playd_tests EXCLUDE_FROM_ALL ${SRCS} ${tests_SRCS})
target_compile_features(playd_tests PUBLIC cxx_std_17)

set_target_properties(playd playd_tests
  PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

add_test(NAME playd_tests COMMAND playd_tests)

# `make check` to both compile and run tests
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  set(ctest_opts "--force-new-ctest-process;-C;$(Configuration)")
endif()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} ${ctest_opts}
  DEPENDS playd_tests)
enable_testing()

# Link and include libraries
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
    include_directories(${${mylib}_INCLUDE_DIR})
  endif()
  unset(libs)
endforeach()

# Install
include(installation)

# Set compiler flags
include(set_compiler_flags)
include(set_warning_flags)
playd_set_warning_flags()
