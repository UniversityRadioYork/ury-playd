# See https://github.com/ruslo/sugar/wiki/Cross-platform-warning-suppression
include(cmake_scripts/sugar/cmake/Sugar)
include(sugar_generate_warning_flags)

option(MAKE_WARNINGS_INTO_ERRORS "Make warnings into errors" ON)
if(MAKE_WARNINGS_INTO_ERRORS)
   set(werror "ALL")
endif()

function(playd_set_warning_flags)
  # Determine compiler
  set(is_msvc ${MSVC})
  if (is_msvc)
    return() # Leaving MSVC on the defaults for now
  endif()
  string(COMPARE EQUAL "${CMAKE_CXX_COMPILER_ID}" "Clang" is_clang)
  string(COMPARE EQUAL "${CMAKE_CXX_COMPILER_ID}" "AppleClang" is_apple_clang)
  if(is_clang OR is_apple_clang)
    set(is_clang TRUE)
  else()
    set(is_clang FALSE)
  endif()

  # Generate those flags
  sugar_generate_warning_flags(
    playd_compile_options
    playd_properties
    ENABLE ALL
    TREAT_AS_ERROR ${werror}
  )

  # Reduce -Weverything to -Wextra on Clang
  if(is_clang)
    list(FIND playd_compile_options "-Weverything" _index)
    if(${_index} GREATER -1)
      list(REMOVE_ITEM playd_compile_options "-Weverything")
      list(APPEND playd_compile_options "-Wextra")
    endif()
  endif()

  # Add flags to targets
  foreach(target playd playd_tests)
    set_target_properties(
      ${target}
      PROPERTIES
      ${playd_properties} # important: without quotes (properties: name, value, name, value, ...)
      COMPILE_OPTIONS
      "${playd_compile_options}" # important: need quotes (one argument for COMPILE_OPTIONS)
    )
  endforeach()

  message(STATUS "C/CXX warning flags: ${playd_compile_options}.")
endfunction()
