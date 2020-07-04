cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_LIST_DIR})
  if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting CMAKE_BUILD_TYPE type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
  endif()
endif()

project(smplwav VERSION 0.1.0 LANGUAGES C)

set(SMPLWAV_PUBLIC_INCLUDES smplwav.h smplwav_convert.h smplwav_mount.h smplwav_serialise.h)

add_library(smplwav STATIC ${SMPLWAV_PUBLIC_INCLUDES} src/smplwav.c src/smplwav_convert.c src/smplwav_internal.h src/smplwav_mount.c src/smplwav_serialise.c)
set_property(TARGET smplwav APPEND PROPERTY PUBLIC_HEADER ${SMPLWAV_PUBLIC_INCLUDES})
set_property(TARGET smplwav PROPERTY ARCHIVE_OUTPUT_DIRECTORY "$<$<NOT:$<CONFIG:Release>>:$<CONFIG>>")

if (x${CMAKE_C_COMPILER_ID} STREQUAL "xMSVC")
  set_property(TARGET smplwav APPEND_STRING PROPERTY COMPILE_FLAGS " /W3")
else()
  set_property(TARGET smplwav APPEND_STRING PROPERTY COMPILE_FLAGS " -Wall")
endif()

target_include_directories(smplwav PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>")

find_package(cop CONFIG REQUIRED)
target_link_libraries(smplwav cop)

add_subdirectory(app_sampleauth)

include(CMakePackageConfigHelpers)

configure_package_config_file(cmake_support/smplwavConfig.cmake.in ${CMAKE_BINARY_DIR}/cmake/smplwav/smplwavConfig.cmake
  INSTALL_DESTINATION "lib/cmake/smplwav"
  NO_CHECK_REQUIRED_COMPONENTS_MACRO)
write_basic_package_version_file(${CMAKE_BINARY_DIR}/cmake/smplwav/smplwavConfigVersion.cmake
  COMPATIBILITY SameMajorVersion)

export(TARGETS smplwav FILE "${PROJECT_BINARY_DIR}/cmake/smplwav/smplwavTargets.cmake")

install(TARGETS smplwav
  EXPORT smplwav_targets
  RUNTIME DESTINATION "bin$<$<NOT:$<CONFIG:Release>>:/$<CONFIG>>"
  ARCHIVE DESTINATION "lib$<$<NOT:$<CONFIG:Release>>:/$<CONFIG>>"
  LIBRARY DESTINATION "lib$<$<NOT:$<CONFIG:Release>>:/$<CONFIG>>"
  PUBLIC_HEADER DESTINATION include/smplwav
  INCLUDES DESTINATION include)
install(EXPORT smplwav_targets DESTINATION lib/cmake/smplwav FILE "smplwavTargets.cmake")
install(FILES "${CMAKE_BINARY_DIR}/cmake/smplwav/smplwavConfig.cmake"
              "${CMAKE_BINARY_DIR}/cmake/smplwav/smplwavConfigVersion.cmake"
        DESTINATION "lib/cmake/smplwav")
install(FILES LICENSE DESTINATION share/doc/smplwav)
install(FILES README.md DESTINATION share/doc/smplwav)

