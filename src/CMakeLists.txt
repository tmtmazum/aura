cmake_minimum_required(VERSION 3.15)

project(aura)

include(common-properties.cmake)

if (NOT DEFINED CINDER_INCLUDE_DIR)
    set(CINDER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cinder2/include)
endif()

# Boost used by cinder only
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/boost/include)

add_subdirectory(aura-core)
add_subdirectory(aura-cli)
add_subdirectory(cinder2)
add_subdirectory(aura-cinder)
