cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(app_sampleauth LANGUAGES C)

add_executable(sampleauth app_sampleauth.c)

if (x${CMAKE_C_COMPILER_ID} STREQUAL "xMSVC")
  set_property(TARGET sampleauth APPEND_STRING PROPERTY COMPILE_FLAGS " /W3")
else()
  set_property(TARGET sampleauth APPEND_STRING PROPERTY COMPILE_FLAGS " -Wall")
endif()

target_link_libraries(sampleauth smplwav)

install(TARGETS sampleauth RUNTIME DESTINATION "bin$<$<NOT:$<CONFIG:Release>>:/$<CONFIG>>")
