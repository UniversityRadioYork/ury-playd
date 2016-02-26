# Def if MP3 supported
list (FIND SUPPORTED_FMTS "MP3" _index)
if (${_index} GREATER -1)
  add_sources(mp3.cpp)
endif()

# Def if sndfile found
if(LIBSNDFILE_FOUND)
  add_sources(sndfile.cpp)
endif()
