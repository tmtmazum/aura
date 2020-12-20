project(aura-core)

file(GLOB aura_core_h *.h)
file(GLOB aura_core_src *.cpp)

if (WIN32)
    file(GLOB platform_src ${CMAKE_CURRENT_SOURCE_DIR}/windows/*.cpp)
elseif (UNIX)
    file(GLOB platform_src ${CMAKE_CURRENT_SOURCE_DIR}/linux/*.cpp)
endif()

add_library(aura_core STATIC ${aura_core_h} ${aura_core_src} ${platform_src})
target_link_libraries(aura_core aura_utils)
target_compile_features(aura_core PUBLIC cxx_std_17)

foreach(header ${aura_core_h})
  list(APPEND copy_public_includes
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${header}  ${CMAKE_SOURCE_DIR}/include/aura)
endforeach()

add_custom_target(copy_aura_core
  COMMENT "Copying public header files -> include/aura"
  ${copy_public_includes}
)

set_aura_binary_properties(aura_core)
add_dependencies(aura_utils copy_aura_core)
