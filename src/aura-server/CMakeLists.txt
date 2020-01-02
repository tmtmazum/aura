project(aura-server)

file(GLOB aura_server_src *.cpp *.h)

add_executable(aura_server ${aura_server_src})
target_link_libraries(aura_server aura_core cinder)
target_include_directories(aura_server PRIVATE ${CINDER_INCLUDE_DIR})
if (MSVC)
    target_link_options(aura_server PRIVATE)
endif()
