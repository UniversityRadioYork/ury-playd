# Compiler specific flags
include(CheckCXXCompilerFlag)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  # using Visual Studio C++
  # Linker
  # /OPT:REF enables also /OPT:ICF and disables INCREMENTAL
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF")
  # /OPT:NOICF is recommended when /DEBUG is used (http://msdn.microsoft.com/en-us/library/xe4t6fc1.aspx)
  set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:NOICF")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF")
  set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:NOICF")
endif(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  if(MSVC_VERSION STREQUAL "1900")
    set(SUPPORTS_C14 ON)
  endif(MSVC_VERSION STREQUAL "1900")
else(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  CHECK_CXX_COMPILER_FLAG("-std=c++14" SUPPORTS_C14)
  if(SUPPORTS_C14)
    message(STATUS "Enabling '-std=c++14' C++ compiler flag.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
  endif(SUPPORTS_C14)
endif(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)

if(SUPPORTS_C14)
  message(STATUS "Using C++14 compatible compiler: ${CMAKE_CXX_COMPILER_ID}.")
else(SUPPORTS_C14)
  message(FATAL_ERROR "Unable to locate a C++14 compatible compiler.")
endif(SUPPORTS_C14)


# This flag appeared to be important when compiling in Travis VMs
CHECK_CXX_COMPILER_FLAG(-fPIC CXX_FPIC_FLAG)
if(CXX_FPIC_FLAG)
  message(STATUS "Enabling '-fPIC' C compiler flag.")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif(CXX_FPIC_FLAG)

# Print compiler flags
message(STATUS "CXX compilation flags: ${CMAKE_CXX_FLAGS}.")
message(STATUS "C compilation flags: ${CMAKE_C_FLAGS}.")
