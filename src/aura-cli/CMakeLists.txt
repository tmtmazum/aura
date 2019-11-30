project(aura-cli)

file(GLOB aura_cli_src *.cpp *.h)

add_executable(aura_cli ${aura_cli_src})
target_link_libraries(aura_cli aura_core)
