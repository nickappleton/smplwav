cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_LIST_DIR})
  if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting CMAKE_BUILD_TYPE type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
  endif()
  project(smplwav VERSION 0.1.0 LANGUAGES C)
endif()

add_library(smplwav STATIC ../smplwav.h ../smplwav_convert.h ../smplwav_mount.h ../smplwav_serialise.h smplwav.c smplwav_convert.c smplwav_internal.h smplwav_mount.c smplwav_serialise.c)

if (x${CMAKE_CXX_COMPILER_ID} STREQUAL "xMSVC")
  set_property(TARGET smplwav APPEND_STRING PROPERTY COMPILE_FLAGS " /W3")
else()
  set_property(TARGET smplwav APPEND_STRING PROPERTY COMPILE_FLAGS " -Wall")
endif()

target_include_directories(smplwav PRIVATE "../..")
target_link_libraries(smplwav cop)

