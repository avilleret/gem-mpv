cmake_minimum_required (VERSION 3.0)
project(gem_mpv)

set(CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_LIST_DIR}/CMake-modules)
set(CMAKE_PREFIX_PATH "/home/antoine/dev/mpv/build/install/")

find_package(MPV REQUIRED)

message(STATUS "MPV include dir: ${MPV_INCLUDE_DIR}")
message(STATUS "MPV lib : ${MPV_LIBRARY}")

set(SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/gem_mpv.cpp
  ${CMAKE_CURRENT_LIST_DIR}/gemframebuffer.cpp
)

set(HEADERS
  ${CMAKE_CURRENT_LIST_DIR}/gem_mpv.hpp
  ${CMAKE_CURRENT_LIST_DIR}/gemframebuffer.hpp
)

include_directories(
  /usr/local/include/Gem
  ${MPV_INCLUDE_DIR}
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# set library binary file name and extension
set_target_properties(${PROJECT_NAME}
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME "mpv")

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".pd_darwin")
elseif(UNIX)
    if(${OSSIA_ARCHITECTURE} MATCHES "arm")
        set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".l_arm")
    elseif(${OSSIA_ARCHITECTURE} MATCHES "x86")
        set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".l_i386")
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".pd_linux")
    endif()
endif()

target_link_libraries(${PROJECT_NAME} ${MPV_LIBRARY})
