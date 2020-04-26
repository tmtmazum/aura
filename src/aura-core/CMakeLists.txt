project(aura-core)

file(GLOB aura_core_src *.h *.cpp)

if (WIN32)
    file(GLOB platform_src ${CMAKE_CURRENT_SOURCE_DIR}/windows/*.cpp)
elseif (UNIX)
    file(GLOB platform_src ${CMAKE_CURRENT_SOURCE_DIR}/linux/*.cpp)
endif()

add_library(aura_core STATIC ${aura_core_src} ${platform_src})
target_compile_features(aura_core PUBLIC cxx_std_17)
