add_sources(playd
  cmd_result.cpp
  errors.cpp
  io.cpp
  main.cpp
  player.cpp
  response.cpp
  tokeniser.cpp
  )

add_subdirectory("audio")
add_subdirectory("audio/sources")
add_subdirectory("contrib")

add_subdirectory("tests")