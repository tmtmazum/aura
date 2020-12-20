cmake_minimum_required(VERSION 3.15)

project(aura)


include(src/common-properties.cmake)

add_compile_definitions($<IF:$<CONFIG:Debug>,AURA_DEBUG=1,AURA_DEBUG=0>)

include_directories(external/boost)
link_directories(external/boost/stage/lib)

include_directories(external/Cinder/include)
include_directories(include)

add_subdirectory(src/aura-utils)
add_subdirectory(src/aura-core)
add_subdirectory(src/aura-game)

add_subdirectory(external/Cinder)

add_subdirectory(src/aura-server)
# add_subdirectory(src/aura-client)

add_subdirectory(test/ogl-test)
