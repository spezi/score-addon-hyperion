cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(score_addon_hyperion LANGUAGES CXX)

if(NOT TARGET score_lib_base)
  include("${SCORE_SOURCE_DIR}/cmake/ScoreExternalAddon.cmake")
else()
  if(NOT TARGET score_plugin_gfx)
    return()
  endif()
endif()

if(EMSCRIPTEN)
  return()
endif()

# General initialization
score_common_setup()

# Find FlatBuffers compiler - prefer /usr/local (user-installed) over system
find_program(FLATC_EXECUTABLE flatc
  HINTS
    /usr/local/bin
    /usr/bin
)

if(NOT FLATC_EXECUTABLE)
  message(FATAL_ERROR "flatc compiler not found. Please install flatbuffers-compiler.")
endif()

message(STATUS "Using flatc: ${FLATC_EXECUTABLE}")

# Generate FlatBuffers headers at build time
set(FBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/fbs)
set(FBS_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${FBS_GENERATED_DIR})

set(FBS_FILES
  ${FBS_DIR}/hyperion_request.fbs
)

set(FBS_GENERATED_HEADERS
  ${FBS_GENERATED_DIR}/hyperion_request_generated.h
)

add_custom_command(
  OUTPUT ${FBS_GENERATED_HEADERS}
  COMMAND ${FLATC_EXECUTABLE} --cpp -o ${FBS_GENERATED_DIR} ${FBS_FILES}
  DEPENDS ${FBS_FILES}
  COMMENT "Generating FlatBuffers headers for Hyperion"
)

add_custom_target(hyperion_flatbuffers_generate DEPENDS ${FBS_GENERATED_HEADERS})

# Creation of the library
add_library(score_addon_hyperion
  Hyperion/OutputNode.hpp
  Hyperion/OutputFactory.hpp
  Hyperion/OutputSettings.hpp
  Hyperion/HyperionConnection.hpp

  Hyperion/OutputNode.cpp
  Hyperion/OutputFactory.cpp
  Hyperion/HyperionConnection.cpp

  score_addon_hyperion.hpp
  score_addon_hyperion.cpp
)

add_dependencies(score_addon_hyperion hyperion_flatbuffers_generate)

# CRITICAL: Disable PCH for HyperionConnection.cpp to avoid score/FlatBuffers conflicts
set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/Hyperion/HyperionConnection.cpp
  PROPERTIES
    SKIP_PRECOMPILE_HEADERS ON
)

target_include_directories(score_addon_hyperion 
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FBS_GENERATED_DIR}
)

# Find FlatBuffers include directory - prefer /usr/local (user-installed)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(FLATBUFFERS QUIET flatbuffers)
endif()

if(FLATBUFFERS_INCLUDE_DIRS)
  target_include_directories(score_addon_hyperion PRIVATE ${FLATBUFFERS_INCLUDE_DIRS})
else()
  find_path(FLATBUFFERS_INCLUDE_DIR flatbuffers/flatbuffers.h
    HINTS
      /usr/local/include
      /usr/include
      ${CMAKE_PREFIX_PATH}/include
  )
  if(FLATBUFFERS_INCLUDE_DIR)
    target_include_directories(score_addon_hyperion PRIVATE ${FLATBUFFERS_INCLUDE_DIR})
    message(STATUS "Using FlatBuffers headers: ${FLATBUFFERS_INCLUDE_DIR}")
  else()
    message(FATAL_ERROR "FlatBuffers headers not found. Please install libflatbuffers-dev.")
  endif()
endif()

# Link
target_link_libraries(score_addon_hyperion
  PUBLIC
    score_plugin_engine score_plugin_gfx
)

# Target-specific options
setup_score_plugin(score_addon_hyperion)
