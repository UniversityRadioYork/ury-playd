# Setting compiler secific flags
INCLUDE(CheckCXXCompilerFlag)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

	CHECK_CXX_COMPILER_FLAG(-ftemplate-depth=256 CLANG_TEMPLATE_DEPTH)
	IF(CLANG_TEMPLATE_DEPTH)
		MESSAGE(STATUS "Enabling '-ftemplate-depth=256' compiler flag required since boost 1.54.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=256")
	ENDIF(CLANG_TEMPLATE_DEPTH)

ELSEIF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

	CHECK_CXX_COMPILER_FLAG(fno-strict-aliasing GNUCXX_NO_STRICT_ALIASING)
	IF(GNUCXX_NO_STRICT_ALIASING)
		MESSAGE(STATUS "Enabling '-fno-strict-aliasing' compiler flag.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
	ENDIF(GNUCXX_NO_STRICT_ALIASING)

	CHECK_CXX_COMPILER_FLAG(-Wnon-virtual-dtor GNUCXX_W_ALL)
	IF(GNUCXX_W_ALL)
		MESSAGE(STATUS "Enabling '-Wall' compiler flag.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	ENDIF(GNUCXX_W_ALL)

	CHECK_CXX_COMPILER_FLAG(-Wno-deprecated GNUCXX_W_EXTRA)
	IF(GNUCXX_W_EXTRA)
		MESSAGE(STATUS "Enabling '-Wextra' compiler flag.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
	ENDIF(GNUCXX_W_EXTRA)

	CHECK_CXX_COMPILER_FLAG(-Wno-ignored-qualifiers GNUCXX_W_NOEXCEPT)
	IF(GNUCXX_W_NOEXCEPT)
		MESSAGE(STATUS "Enabling '-Wnoexcept' compiler flag.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnoexcept")
	ENDIF(GNUCXX_W_NOEXCEPT)

	CHECK_CXX_COMPILER_FLAG(-Wno-ignored-qualifiers GNUCXX_W_DISOPT)
	IF(GNUCXX_W_DISOPT)
		MESSAGE(STATUS "Enabling '-Wdisabled-optimization' compiler flag.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdisabled-optimization")
	ENDIF(GNUCXX_W_DISOPT)

	CHECK_CXX_COMPILER_FLAG(-Wno-ignored-qualifiers GNUCXX_FASTMATH)
	IF(GNUCXX_FASTMATH)
		MESSAGE(STATUS "Enabling '-ffast-math' compiler flag.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
	ENDIF(GNUCXX_FASTMATH)

ELSEIF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
	CHECK_CXX_COMPILER_FLAG(-mieee-fp INTEL_IEEE_COMPLIANT)
	IF(INTEL_IEEE_COMPLIANT)
			MESSAGE(STATUS "Enabling '-mieee-fp' compiler flag to get IEEE compliant code")
			SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee-fp")
	ENDIF(INTEL_IEEE_COMPLIANT)
ELSEIF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
    # Linker
    # /OPT:REF enables also /OPT:ICF and disables INCREMENTAL
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF")
    # /OPT:NOICF is recommended when /DEBUG is used (http://msdn.microsoft.com/en-us/library/xe4t6fc1.aspx)
    set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:NOICF")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:NOICF")
ENDIF()

# The following flags will be tried on all compilers
# If a C++14 able compiler is detected, then the flag is added
# otherwise an error is thrown
CHECK_CXX_COMPILER_FLAG(-std=c++14 ALL_C14)
IF(ALL_C14)
	MESSAGE(STATUS "Enabling '-std=c++14' compiler flag")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
ENDIF(ALL_C14)

IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	IF(MSVC_VERSION STREQUAL "1900")
		MESSAGE(STATUS "Using MSVC 2015. Supports C++14.")
		SET(ALL_C14 ON)
	ENDIF(MSVC_VERSION STREQUAL "1900")
ENDIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

IF(NOT ALL_C14)
  MESSAGE(FATAL_ERROR "Unable to locate a c++14 compatible compiler")
ENDIF(NOT ALL_C14)


# This flag appeared to be important when compiling in Travis VMs
CHECK_CXX_COMPILER_FLAG(-fPIC CXX_FPIC_FLAG)
#IF(CXX_FPIC_FLAG)
#	MESSAGE(STATUS "Enabling '-fPIC' compiler flag")
#	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
#ENDIF(CXX_FPIC_FLAG)

#CHECK_C_COMPILER_FLAG(-fPIC C_FPIC_FLAG)
IF(CXX_FPIC_FLAG)
	MESSAGE(STATUS "Enabling '-fPIC' compiler flag")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
ENDIF(CXX_FPIC_FLAG)

# We finally log the compiler flags
MESSAGE(STATUS "CXX compilation flags: ${CMAKE_CXX_FLAGS}")
MESSAGE(STATUS "C compilation flags: ${CMAKE_C_FLAGS}")
