cmake_minimum_required(VERSION 3.15)

project(aura)

include(src/common-properties.cmake)

add_compile_definitions($<IF:$<CONFIG:Debug>,AURA_DEBUG=1,AURA_DEBUG=0>)

# Boost used by cinder only
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/boost/include)
include_directories(external/Cinder-Boost)
include_directories(external/Cinder/include)

add_subdirectory(src/aura-core)
add_subdirectory(src/aura-cli)
# add_subdirectory(cinder2)
add_subdirectory(external/Cinder)
add_subdirectory(src/aura-cinder)
# add_subdirectory(src/aura-server)
# add_subdirectory(src/aura-client)

# add_subdirectory(src/cpp-httplib)
