FILE(GLOB_RECURSE lib_sources ${CMAKE_SOURCE_DIR}/lib/*.*)

idf_component_register(SRCS ${lib_sources})