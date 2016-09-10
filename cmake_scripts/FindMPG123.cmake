# Copyright (c) kcat (https://github.com/kcat), 2014
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the <organization> nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# - Find mpg123
# Find the native mpg123 includes and library
#
#  MPG123_INCLUDE_DIR - where to find mpg123.h
#  MPG123_LIBRARIES   - List of libraries when using mpg123.
#  MPG123_FOUND       - True if mpg123 found.

IF(MPG123_INCLUDE_DIR AND MPG123_LIBRARIES)
  # Already in cache, be silent
  SET(MPG123_FIND_QUIETLY TRUE)
ENDIF(MPG123_INCLUDE_DIR AND MPG123_LIBRARIES)

FIND_PATH(MPG123_INCLUDE_DIR mpg123.h
          PATHS "${MPG123_DIR}"
          PATH_SUFFIXES include
          )

FIND_LIBRARY(MPG123_LIBRARIES NAMES mpg123 mpg123-0 libmpg123 libmpg123-0
             PATHS "${MPG123_DIR}"
             PATH_SUFFIXES lib
             )


# handle the QUIETLY and REQUIRED arguments and set MPG123_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MPG123 DEFAULT_MSG MPG123_LIBRARIES MPG123_INCLUDE_DIR)