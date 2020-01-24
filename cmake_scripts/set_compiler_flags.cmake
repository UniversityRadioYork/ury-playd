# Compiler specific flags
include(CheckCXXCompilerFlag)

IF (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    # using Visual Studio C++
    # Linker
    # /OPT:REF enables also /OPT:ICF and disables INCREMENTAL
    SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF")
    # /OPT:NOICF is recommended when /DEBUG is used (http://msdn.microsoft.com/en-us/library/xe4t6fc1.aspx)
    SET(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:NOICF")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF")
    SET(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:NOICF")
	# /LTCG for cross-module inlining
    set_target_properties(playd playd_tests PROPERTIES INTERPROCEDURAL_OPTIMIZATION True)
ENDIF(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)

MESSAGE(STATUS "Using compiler: ${CMAKE_CXX_COMPILER_ID}.")

# This flag appeared to be important when compiling in Travis VMs
CHECK_CXX_COMPILER_FLAG(-fPIC CXX_FPIC_FLAG)
if(CXX_FPIC_FLAG)
  message(STATUS "Enabling '-fPIC' C compiler flag.")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

# Print compiler flags
message(STATUS "CXX compilation flags: ${CMAKE_CXX_FLAGS}.")
message(STATUS "C compilation flags: ${CMAKE_C_FLAGS}.")
