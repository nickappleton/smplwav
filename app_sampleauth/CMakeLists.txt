cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting CMAKE_BUILD_TYPE type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

project(app_sampleauth)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -mmacosx-version-min=10.6")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
endif()

add_executable(sampleauth app_sampleauth.c)
target_include_directories(sampleauth PRIVATE "../..")
target_link_libraries(sampleauth smplwav)

add_subdirectory("../../cop" "${CMAKE_CURRENT_BINARY_DIR}/cop_dep")
add_subdirectory("../../smplwav" "${CMAKE_CURRENT_BINARY_DIR}/smplwav_dep")
