# TODO: Manual versioning?
execute_process(COMMAND "git" "describe" "--tags" "--always" WORKING_DIRECTORY "${playd_SOURCE_DIR}" RESULT_VARIABLE NOT_GIT_REPO OUTPUT_VARIABLE PROGVER OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT_GIT_REPO)
  message(FATAL "Not a git repo")
else()
  add_definitions(-DPD_VERSION="${PROGVER}")
endif()
